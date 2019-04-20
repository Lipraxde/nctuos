#include <inc/syscall.h>
#include <inc/trap.h>
#include <kernel/timer.h>

// kernel/screen.c
extern void putch(unsigned char c);

// kernel/kbd.c
extern int getc(void);

static void
do_puts(char *str, uint32_t len)
{
	uint32_t i;
	for (i = 0; i < len; i++)
	{
		putch(str[i]);
	}
}

static int32_t
do_getc(void)
{
	return getc();
}

static int32_t
do_syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	int32_t retVal = -1;

	switch (syscallno) {
	case SYS_fork:
		/* TODO: Lab 5
		* You can reference kernel/task.c, kernel/task.h
		*/
		break;
	case SYS_getc:
		retVal = do_getc();
		break;
	case SYS_puts:
		do_puts((char*)a1, a2);
		retVal = 0;
		break;
	case SYS_getpid:
		/* TODO: Lab 5
		* Get current task's pid
		*/
		break;
	case SYS_sleep:
		/* TODO: Lab 5
		* Yield this task
		* You can reference kernel/sched.c for yielding the task
		*/
		break;
	case SYS_kill:
		/* TODO: Lab 5
		* Kill specific task
		* You can reference kernel/task.c, kernel/task.h
		*/
		break;
	case SYS_get_num_free_page:
		/* TODO: Lab 5
		 * You can reference kernel/mem.c
		 */
		break;
	case SYS_get_num_used_page:
		/* TODO: Lab 5
		 * You can reference kernel/mem.c
		 */
		break;
	case SYS_get_ticks:
		/* TODO: Lab 5
		 * You can reference kernel/timer.c
		 */
		retVal = get_tick();
		break;
	case SYS_settextcolor:
		/* TODO: Lab 5
		 * You can reference kernel/screen.c
		 */
		break;
	case SYS_cls:
		/* TODO: Lab 5
		 * You can reference kernel/screen.c
		 */
		break;
	default:
		return -1;
	}
	return retVal;
}

void
syscall_dispatch(struct Trapframe *tf)
{
	tf->tf_regs.reg_eax = do_syscall(tf->tf_regs.reg_eax,
		tf->tf_regs.reg_edx,
		tf->tf_regs.reg_ecx,
		tf->tf_regs.reg_ebx,
		tf->tf_regs.reg_edi,
		tf->tf_regs.reg_esi);
}
