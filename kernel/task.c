#include <inc/elf.h>
#include <inc/error.h>
#include <inc/mmu.h>
#include <inc/types.h>
#include <inc/string.h>
#include <inc/x86.h>
#include <inc/memlayout.h>
#include <kernel/cpu.h>
#include <kernel/task.h>
#include <kernel/mem.h>

// Global descriptor table.
//
// Set up global descriptor table (GDT) with separate segments for
// kernel mode and user mode.  Segments serve many purposes on the x86.
// We don't use any of their memory-mapping capabilities, but we need
// them to switch privilege levels. 
//
// The kernel and user segments are identical except for the DPL.
// To load the SS register, the CPL must equal the DPL.  Thus,
// we must duplicate the segments for the user and the kernel.
//
// In particular, the last argument to the SEG macro used in the
// definition of gdt specifies the Descriptor Privilege Level (DPL)
// of that descriptor: 0 for kernel and 3 for user.
//
static struct Segdesc gdt[NCPU + 5] =
{
	// 0x0 - unused (always faults -- for trapping NULL far pointers)
	SEG_NULL,

	// 0x8 - kernel code segment
	[GD_KT >> 3] = SEG(STA_X | STA_R, 0x0, 0xffffffff, 0),

	// 0x10 - kernel data segment
	[GD_KD >> 3] = SEG(STA_W, 0x0, 0xffffffff, 0),

	// 0x18 - user code segment
	[GD_UT >> 3] = SEG(STA_X | STA_R, 0x0, 0xffffffff, 3),

	// 0x20 - user data segment
	[GD_UD >> 3] = SEG(STA_W , 0x0, 0xffffffff, 3),

	// First TSS descriptors (starting from GD_TSS0) are initialized
	// in task_init()
	[GD_TSS0 >> 3] = SEG_NULL
	
};

static struct Pseudodesc gdt_pd = {
	sizeof(gdt) - 1, (unsigned long) gdt
};

static struct Task *task_free_list;
struct Task tasks[NR_TASKS];

extern char bootstacktop[];

int idle_entry()
{
/*
	asm volatile("movl %0,%%eax\n\t" \
			"movw %%ax,%%ds\n\t" \
			"movw %%ax,%%es\n\t" \
			"movw %%ax,%%fs\n\t" \
			"movw %%ax,%%gs" \
			:: "i" (0x20 | 0x03));
*/
	while(1);
}

//
// Initialize the kernel virtual memory layout for environment e.
// Allocate a page directory, set e->env_pgdir accordingly,
// and initialize the kernel portion of the new environment's address space.
// Do NOT (yet) map anything into the user portion
// of the environment's virtual address space.
//
// Returns 0 on success, < 0 on error.  Errors include:
//	-E_NO_MEM if page directory or table could not be allocated.
//
static int
setupkvm(struct Task *t)
{
	int i;
	struct PageInfo *p = NULL;

	// Allocate a page for the page directory
	if (!(p = page_alloc(ALLOC_ZERO)))
		return -E_NO_MEM;

	// Hint:
	//    - The VA space of all envs is identical above UTOP
	//	(except at UVPT, which we've set below).
	//	See inc/memlayout.h for permissions and layout.
	//	Can you use kern_pgdir as a template?  Hint: Yes.
	//	(Make sure you got the permissions right in Lab 2.)
	//    - The initial VA below UTOP is empty.
	//    - You do not need to make any more calls to page_alloc.
	//    - Note: In general, pp_ref is not maintained for
	//	physical pages mapped only above UTOP, but env_pgdir
	//	is an exception -- you need to increment env_pgdir's
	//	pp_ref for env_free to work correctly.
	//    - The functions in kern/pmap.h are handy.
	t->pgdir = (pde_t *)page2kva(p);
	for (i = 0; i < NPDENTRIES; i++) {
		t->pgdir[i] = kern_pgdir[i];
		if (kern_pgdir[i] & PTE_P)
			pa2page(PTE_ADDR(kern_pgdir[i]))->pp_ref++;
	}

	// UVPT maps the env's own page table read-only.
	// Permissions: kernel R, user R
	t->pgdir[PDX(UVPT)] = PADDR(t->pgdir) | PTE_P | PTE_U;
	p->pp_ref++;

	return 0;
}

// XXX: Now just map to physical address
static void
setupvm(pde_t *pgdir, uintptr_t va, size_t size, physaddr_t pa)
{
	size_t i;
	for (i = 0; i < size; i += PGSIZE) {
		pte_t *pte = pgdir_walk(pgdir, (char *)va, 1);
		*pte = pa | PTE_W | PTE_U | PTE_P;
		// Why I need this? I do not find in TA code @@
		pgdir[PDX(va)] |= PTE_U;
		va += PGSIZE;
		pa += PGSIZE;
	}
}

static void
region_alloc(struct Task *ts, void *va, size_t len)
{
	size_t size = ROUNDUP(va + len, PGSIZE) - ROUNDDOWN(va, PGSIZE);
	int i;
	for (i = 0; i < size; i += PGSIZE) {
		// XXX
		struct PageInfo *page = page_alloc(0);
		if (!page)
			panic("No page");
		page_insert(ts->pgdir, page, va + i, PTE_P | PTE_W | PTE_U);
	}
}

/* TODO: Lab5
 * 1. Find a free task structure for the new task,
 *    the global task list is in the array "tasks".
 *    You should find task that is in the state "TASK_FREE"
 *    If cannot find one, return -1.
 *
 * 2. Setup the page directory for the new task
 *
 * 3. Setup the user stack for the new task, you can use
 *    page_alloc() and page_insert(), noted that the va
 *    of user stack is started at USTACKTOP and grows down
 *    to USR_STACK_SIZE, remember that the permission of
 *    those pages should include PTE_U
 *
 * 4. Setup the Trapframe for the new task
 *    We've done this for you, please make sure you
 *    understand the code.
 *
 * 5. Setup the task related data structure
 *    You should fill in task_id, state, parent_id,
 *    and its schedule time quantum (remind_ticks).
 *
 * 6. Return the pid of the newly created task.
 *
 */
static int task_create(bool is_u)
{
	struct Task *ts = NULL;

	/* Find a free task structure */
	if (task_free_list == NULL)
		return -1;
	ts = task_free_list;
	task_free_list = ts->task_link;

	// /* Setup Page Directory and pages for kernel*/
	if (setupkvm(ts))
		panic("Not enough memory for per process page directory!\n");

	/* Setup User Stack */
	region_alloc(ts, (void *)(USTACKTOP - USR_STACK_SIZE), USR_STACK_SIZE);

	/* Setup Trapframe */
	memset( &(ts->tf), 0, sizeof(ts->tf));

	if (is_u) {
		ts->tf.tf_cs = GD_UT | 0x03;
		ts->tf.tf_ds = GD_UD | 0x03;
		ts->tf.tf_es = GD_UD | 0x03;
		ts->tf.tf_ss = GD_UD | 0x03;
	} else {
		ts->tf.tf_cs = GD_KT | 0x00;
		ts->tf.tf_ds = GD_KD | 0x00;
		ts->tf.tf_es = GD_KD | 0x00;
		ts->tf.tf_ss = GD_KD | 0x00;
	}
	ts->tf.tf_esp = USTACKTOP;
	ts->tf.tf_eflags = FL_IF;

	// /* Setup task structure (task_id and parent_id) */
	// ts->task_id = ts - tasks;	// setup at init
	// ts->parent_id = 0;		// setup at fork

	return (ts - tasks);
}


/* TODO: Lab5
 * This function free the memory allocated by kernel.
 *
 * 1. Be sure to change the page directory to kernel's page
 *    directory to avoid page fault when removing the page
 *    table entry.
 *    You can change the current directory with lcr3 provided
 *    in inc/x86.h
 *
 * 2. You have to remove pages of USER STACK
 *
 * 3. You have to remove pages of page table
 *
 * 4. You have to remove pages of page directory
 *
 * HINT: You can refer to page_remove, ptable_remove, and pgdir_remove
 */
static void task_free(int pid)
{
	lcr3(PADDR(kern_pgdir));
	struct Task *ts = &tasks[pid];
	// Remove stack
	uint32_t i = USTACKTOP - USR_STACK_SIZE;
	for(; i < USTACKTOP; i += PGSIZE){
		page_remove(ts->pgdir, i);
	}
	// Remove page table
	for (i = 0; i < NPDENTRIES; i++) {
		if (ts->pgdir[i] & PTE_P)
			page_decref(pa2page(PTE_ADDR(ts->pgdir[i])));
	}
	// Remove page directory
	// We map pgdir to UVPT, remove page table will remove pgdir
	// Umm... magic work! OwO
	// page_free(pa2page(PADDR(ts->pgdir)));
	ts->pgdir = NULL;
}

// Lab6 TODO
//
// Modify it so that the task will be removed
// ( we not implement signal yet so do not try to kill process
// running on other cpu )
//
void sys_kill(int pid)
{
	if (pid == 0)
		pid = thiscpu->cpu_task->task_id;
	if (pid > 0 && pid < NR_TASKS)
	{
		struct Task *t = &tasks[pid];
		task_free(pid);
		t->state = TASK_FREE;
		t->task_link = task_free_list;
		task_free_list = t;
		// debug_page = false;
		sched_yield();
	}
}

/* TODO: Lab 5
 * In this function, you have several things todo
 *
 * 1. Use task_create() to create an empty task, return -1
 *    if cannot create a new one.
 *
 * 2. Copy the trap frame of the parent to the child
 *
 * 3. Copy the content of the old stack to the new one,
 *    you can use memcpy to do the job. Remember all the
 *    address you use should be virtual address.
 *
 * 4. Setup virtual memory mapping of the user prgram 
 *    in the new task's page table.
 *    According to linker script, you can determine where
 *    is the user program. We've done this part for you,
 *    but you should understand how it works.
 *
 * 5. The very important step is to let child and 
 *    parent be distinguishable!
 *
 * HINT: You should understand how system call return
 * it's return value.
 */
int sys_fork()
{
	/* pid for newly created process */
	int pid;
	
	if ((uint32_t)thiscpu->cpu_task)
	{
		// debug_page = true;
		pid = task_create(true);
		
		if (pid < 0)
			return -1;

		// Copy trapframe
		tasks[pid].tf = thiscpu->cpu_task->tf;
		// Copy stack
		uint32_t i = USTACKTOP - USR_STACK_SIZE;
		for(; i < USTACKTOP; i += PGSIZE){
			struct PageInfo *pp = page_lookup(tasks[pid].pgdir, i , NULL);
			memcpy((page2kva(pp)), i, PGSIZE);
		}
		// Setup virtual memory
		setupvm(tasks[pid].pgdir, 0x800000, 64*PGSIZE, 0x800000);
		// Setup child is runnable
		tasks[pid].state = TASK_RUNNABLE;
		// Child return 0
		tasks[pid].tf.tf_regs.reg_eax = 0;
		// Setup child parent
		tasks[pid].parent_id = thiscpu->cpu_task->task_id;
		return pid;
	}

	panic("fork but thiscpu->cpu_task not exist!");
}

/* TODO: Lab5
 * We've done the initialization for you,
 * please make sure you understand the code.
 */
void task_init(void)
{
	int i;

	/* Initial task sturcture */
	task_free_list = NULL;
	for (i = NR_TASKS - 1; i >= 0; --i)
	{
		memset(&(tasks[i]), 0, sizeof(struct Task));
		tasks[i].state = TASK_FREE;
		tasks[i].task_link = task_free_list;
		tasks[i].task_id = i;
		task_free_list = &tasks[i];
	}
}

//
// Lab6 TODO
//
// Please modify this function to:
//
// 1. init idle task for non-booting AP 
//    (remember to put the task in cpu runqueue) 
//
// 2. init per-CPU Runqueue
//
// 3. init per-CPU system registers
//
// 4. init per-CPU TSS
//
struct Task *
task_init_percpu(struct Elf *ehdr) {
	int c = cpunum();
	// Setup a TSS so that we get the right stack
	// when we trap to the kernel.
	memset(&(cpus[c].cpu_tss), 0, sizeof(struct tss_struct));
	// Stack QAQ
	cpus[c].cpu_tss.ts_esp0 = (uint32_t)(&percpu_kstacks[c]) + KSTKSIZE;
	cpus[c].cpu_tss.ts_ss0 = GD_KD;

	// fs and gs stay in user data segment
	cpus[c].cpu_tss.ts_fs = GD_UD | 0x03;
	cpus[c].cpu_tss.ts_gs = GD_UD | 0x03;

	/* Setup TSS in GDT */
	gdt[(GD_TSS0 >> 3) + c] = SEG16(STS_T32A, (uint32_t)(&cpus[c].cpu_tss), sizeof(struct tss_struct), 0);
	gdt[(GD_TSS0 >> 3) + c].sd_s = 0;

	/* Load GDT&LDT */
	lgdt(&gdt_pd);
	lldt(0);

	// Load the TSS selector 
	ltr(GD_TSS0 + (c << 3));

	struct Task *ret;

	// open user task
	if (ehdr) {
		/* Setup first task */
		int i;
		i = task_create(false);
		if (i == -1)
			panic("create task fail");
		ret = &tasks[i];
		// // defalut idle task
		// ret->tf.tf_eip = idle_entry;

		/* For user program */
		setupvm(ret->pgdir, 0x800000, 64*PGSIZE, 0x800000);
		extern void load_elf(struct Task *t, uint8_t *binary);
		load_elf(ret, ehdr);
		ret->tf.tf_cs = GD_UT | 0x03;
		ret->tf.tf_ds = GD_UD | 0x03;
		ret->tf.tf_es = GD_UD | 0x03;
		ret->tf.tf_ss = GD_UD | 0x03;
		ret->tf.tf_eip = ehdr->e_entry;
	}

	return ret;
}

//
// Restores the register values in the Trapframe with the 'iret' instruction.
// This exits the kernel and starts executing some environment's code.
//
// This function does not return.
//
void
task_pop_tf(struct Trapframe *tf)
{
	asm volatile(
		"\tmovl %0,%%esp\n"
		"\tpopal\n"
		"\tpopl %%es\n"
		"\tpopl %%ds\n"
		"\taddl $0x8,%%esp\n" /* skip tf_trapno and tf_errcode */
		"\tiret\n"
		: : "g" (tf) : "memory");
	panic("iret failed");  /* mostly to placate the compiler */
}

void
task_run(struct Task *ts)
{
	if (cur_task && cur_task->state == TASK_RUNNING) {
		cur_task->state = TASK_RUNNABLE;
	}
	cur_task = ts;
	cur_task->state = TASK_RUNNING;
	cur_task->remind_ticks = TIME_QUANT;

	lcr3(PADDR(cur_task->pgdir));

	task_pop_tf(&(cur_task->tf));
}
