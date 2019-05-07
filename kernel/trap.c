#include <kernel/cpu.h>
#include <kernel/task.h>
#include <kernel/trap.h>
#include <inc/assert.h>
#include <inc/mmu.h>
#include <inc/x86.h>

extern void default_trap_entry();	// trap_entry.S
extern void default_errtrap_entry();	// trap_entry.S
extern void pgflt_trap_entry();		// trap_entry.S
extern void kbd_trap_entry();		// trap_entry.S
extern void timer_trap_entry();		// trap_entry.S
extern void syscall_trap_entry();	// trap_entry.S
extern void timer_handler();
extern void kbd_intr();
extern void syscall_dispatch(struct Trapframe *tf);

/* For debugging, so print_trapframe can distinguish between printing
 * a saved trapframe and printing the current trapframe and print some
 * additional information in the latter case.
 */
// use thiscpu->last_tf

struct Gatedesc idt[256] = {{0}};

struct Pseudodesc idt_pd = {sizeof(idt)-1, (uint32_t)idt};

/* For debugging */
static const char *trapname(int trapno)
{
	static const char * const excnames[] = {
		"Divide error",
		"Debug",
		"Non-Maskable Interrupt",
		"Breakpoint",
		"Overflow",
		"BOUND Range Exceeded",
		"Invalid Opcode",
		"Device Not Available",
		"Double Fault",
		"Coprocessor Segment Overrun",
		"Invalid TSS",
		"Segment Not Present",
		"Stack Fault",
		"General Protection",
		"Page Fault",
		"(unknown trap)",
		"x87 FPU Floating-Point Error",
		"Alignment Check",
		"Machine-Check",
		"SIMD Floating-Point Exception"
	};

	if (trapno < sizeof(excnames)/sizeof(excnames[0]))
		return excnames[trapno];
	if (trapno == T_SYSCALL)
		return "System call";
	if (trapno >= IRQ_OFFSET && trapno < IRQ_OFFSET + 16)
		return "Hardware Interrupt";
	return "(unknown trap)";
}

/* For debugging */
void
print_trapframe(struct Trapframe *tf)
{
	cprintf("TRAP frame at %p \n");
	print_regs(&tf->tf_regs);
	cprintf("  es   0x----%04x\n", tf->tf_es);
	cprintf("  ds   0x----%04x\n", tf->tf_ds);
	cprintf("  trap 0x%08x %s\n", tf->tf_trapno, trapname(tf->tf_trapno));
	// If this trap was a page fault that just happened
	// (so %cr2 is meaningful), print the faulting linear address.
	if (tf == thiscpu->last_tf && tf->tf_trapno == T_PGFLT)
		cprintf("  cr2  0x%08x\n", rcr2());
	cprintf("  err  0x%08x", tf->tf_err);
	// For page faults, print decoded fault error code:
	// U/K=fault occurred in user/kernel mode
	// W/R=a write/read caused the fault
	// PR=a protection violation caused the fault (NP=page not present).
	if (tf->tf_trapno == T_PGFLT)
		cprintf(" [%s, %s, %s]\n",
			tf->tf_err & 4 ? "user" : "kernel",
			tf->tf_err & 2 ? "write" : "read",
			tf->tf_err & 1 ? "protection" : "not-present");
	else
		cprintf("\n");
	cprintf("  eip  0x%08x\n", tf->tf_eip);
	cprintf("  cs   0x----%04x\n", tf->tf_cs);
	cprintf("  flag 0x%08x\n", tf->tf_eflags);
	if ((tf->tf_cs & 3) != 0) {
		cprintf("  esp  0x%08x\n", tf->tf_esp);
		cprintf("  ss   0x----%04x\n", tf->tf_ss);
	}
}

void
pgflt_handler(struct Trapframe *tf)
{
	cprintf("Page fault @ %p\n", rcr2());
	while (1);
}

/* For debugging */
void
print_regs(struct PushRegs *regs)
{
	cprintf("  edi  0x%08x\n", regs->reg_edi);
	cprintf("  esi  0x%08x\n", regs->reg_esi);
	cprintf("  ebp  0x%08x\n", regs->reg_ebp);
	cprintf("  oesp 0x%08x\n", regs->reg_oesp);
	cprintf("  ebx  0x%08x\n", regs->reg_ebx);
	cprintf("  edx  0x%08x\n", regs->reg_edx);
	cprintf("  ecx  0x%08x\n", regs->reg_ecx);
	cprintf("  eax  0x%08x\n", regs->reg_eax);
}

static void
trap_dispatch(struct Trapframe *tf)
{
	switch (tf->tf_trapno) {
	case T_PGFLT:
		print_trapframe(tf);
		pgflt_handler(tf);
		break;
	case T_SYSCALL:
		syscall_dispatch(tf);
		break;
	case IRQ_OFFSET+IRQ_KBD:
		kbd_intr();
		break;
	case IRQ_OFFSET+IRQ_TIMER:
		timer_handler();
		break;
	default:
		// Unexpected trap: The user process or the kernel has a bug.
		print_trapframe(tf);
		panic("Unexpected trap!");
	}

}

extern bool booted;
/* 
 * Note: This is the called for every interrupt.
 */
void default_trap_handler(struct Trapframe *tf)
{
	if (booted && thiscpu->cpu_task != 0) {
	// if ((tf->tf_cs & 3) == 3) {
		// Trapped from user mode.
		assert(thiscpu->cpu_task);

		// Copy trap frame (which is currently on the stack)
		// into 'curenv->env_tf', so that running the environment
		// will restart at the trap point.
		thiscpu->cpu_task->tf = *tf;
		// The trapframe on the stack should be ignored from here on.
		tf = &thiscpu->cpu_task->tf;
	}

	// Record that tf is the last real trapframe so
	// print_trapframe can print some additional information.
	thiscpu->last_tf = tf;

	// Dispatch based on what type of trap occurred
	trap_dispatch(tf);

	task_pop_tf(thiscpu->last_tf);
}


void trap_init()
{
	int i;

	for (i = 0; i < 256; ++i) {
		bool is_noec = true;
		
		if (i == T_DBLFLT || i == T_TSS || i == T_SEGNP || i == T_STACK ||
			i == T_GPFLT || i == T_PGFLT || i == T_ALIGN)
			is_noec = false;

		if (is_noec)
			SETGATE(idt[i], 0, GD_KT, default_trap_entry, 0);
		else
			SETGATE(idt[i], 0, GD_KT, default_errtrap_entry, 0);
	}

	/* Keyboard interrupt setup */
	SETGATE(idt[IRQ_OFFSET+IRQ_KBD], 0, GD_KT, kbd_trap_entry, 0);
	/* Timer Trap setup */
	SETGATE(idt[IRQ_OFFSET+IRQ_TIMER], 0, GD_KT, timer_trap_entry, 0);

	SETGATE(idt[T_PGFLT], 0, GD_KT, pgflt_trap_entry, 0);

	SETGATE(idt[T_SYSCALL], 0, GD_KT, syscall_trap_entry, 3);

	/* Load IDT */
	lidt(&idt_pd);
}
