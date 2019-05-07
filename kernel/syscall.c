#include <inc/assert.h>
#include <inc/syscall.h>
#include <inc/trap.h>
#include <kernel/cpu.h>
#include <kernel/task.h>
#include <kernel/timer.h>

// kernel/screen.c
extern void putch(unsigned char c);

// kernel/kbd.c
extern int getc(void);

// kernel/sched.c
extern void sched_yield(void);

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
	int32_t retVal = 0;

	switch (syscallno) {
	case SYS_fork:
		retVal = sys_fork();
		break;
	case SYS_getc:
		retVal = do_getc();
		break;
	case SYS_puts:
		do_puts((char*)a1, a2);
		retVal = 0;
		break;
	case SYS_getpid:
		retVal = thiscpu->cpu_task->task_id;
		break;
	case SYS_getcid:
		retVal = cpunum();
		break;
	case SYS_sleep:
		thiscpu->cpu_task->state = TASK_SLEEP;
		thiscpu->cpu_task->pick_tick = get_tick() + a1;
		sched_yield();
		break;
	case SYS_kill:
		sys_kill(a1);
		break;
	case SYS_get_num_free_page:
		retVal = get_num_free_page();
		break;
	case SYS_get_num_used_page:
		retVal = get_num_used_page();
		break;
	case SYS_get_ticks:
		retVal = get_tick();
		break;
	case SYS_settextcolor:
		settextcolor(a1, a2);
		break;
	case SYS_cls:
		cls();
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
