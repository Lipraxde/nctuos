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

	trap_init();
	kbd_init();
	timer_init();
	mem_init();

	// userprog address
	struct Elf *ehdr = (struct Elf *)0xf0000000;
	struct Task *ts = task_init(ehdr);

	/* Test for page fault handler */
	ptr = (int*)(0x12345678);
	// *ptr = 1;

	/* Enable interrupt */
	__asm __volatile("sti");

	/* Move to user mode */
	// Run first task
	cprintf("used: %d, free: %d\n", get_num_used_page(), get_num_free_page());
	task_run(ts);

	panic("Kernel exit!!");
}
