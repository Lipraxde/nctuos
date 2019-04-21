#include <inc/assert.h>
#include <inc/elf.h>
#include <inc/stdio.h>
#include <inc/kbd.h>
#include <inc/x86.h>
#include <kernel/task.h>
#include <kernel/timer.h>
#include <kernel/trap.h>
#include <kernel/picirq.h>

extern void init_video(void);
extern struct Elf *load_elf(uint32_t pa, uint32_t offset);

void kernel_main(void)
{
	int *ptr;
	init_video();

	pic_init();

	kbd_init();
	timer_init();
	trap_init();
	mem_init();

	// userprog address
	struct Elf *ehdr = (struct Elf *)0xf0000000;
	task_init(ehdr);

	/* Enable interrupt */
	__asm __volatile("sti");

	/* Test for page fault handler */
	ptr = (int*)(0x12345678);
	// *ptr = 1;

	// Load cur_task pgdir
	lcr3(PADDR(cur_task->pgdir));

	/* Move to user mode */
	asm volatile("movl %0,%%eax\n\t" \
	"pushl %1\n\t" \
	"pushl %%eax\n\t" \
	"pushfl\n\t" \
	"pushl %2\n\t" \
	"pushl %3\n\t" \
	"iret\n" \
	:: "m" (cur_task->tf.tf_esp), "i" (GD_UD | 0x03), "i" (GD_KT | 0x00), "m" (cur_task->tf.tf_eip)
	:"ax");
	// :: "m" (cur_task->tf.tf_esp), "i" (GD_UD | 0x03), "i" (GD_UT | 0x03), "m" (cur_task->tf.tf_eip)
	panic("Kernel exit!!");
}
