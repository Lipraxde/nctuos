#include <inc/assert.h>
#include <inc/elf.h>
#include <inc/stdio.h>
#include <inc/kbd.h>
#include <inc/timer.h>
#include <inc/x86.h>
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

	/* Enable interrupt */
	__asm __volatile("sti");

	/* Test for page fault handler */
	ptr = (int*)(0x12345678);
	// *ptr = 1;

	cprintf("Success");
	// shell();
}
