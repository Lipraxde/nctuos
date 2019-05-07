/* Reference: http://www.osdever.net/bkerndev/Docs/pit.htm */
#include <kernel/cpu.h>
#include <kernel/task.h>
#include <kernel/trap.h>
#include <kernel/picirq.h>
#include <inc/mmu.h>
#include <inc/x86.h>

#define TIME_HZ 100

static unsigned long jiffies = 0;

void set_timer(int hz)
{
    int divisor = 1193180 / hz;       /* Calculate our divisor */
    outb(0x43, 0x36);             /* Set our command byte 0x36 */
    outb(0x40, divisor & 0xFF);   /* Set low byte of divisor */
    outb(0x40, divisor >> 8);     /* Set high byte of divisor */
}

/* 
 * Timer interrupt handler
 */
void timer_handler()
{
	if (cpunum() == 0)
		jiffies++;

	if (thiscpu->cpu_task == NULL)
		return;

	/* TODO: Lab 5
	 * 1. Maintain the status of slept tasks
	 * 
	 * 2. Change the state of the task if needed
	 *
	 * 3. Maintain the time quantum of the current task
	 *
	 * 4. sched_yield() if the time is up for current task
	 *
	 */
	struct Task *ts;
	for (ts = &tasks[0]; ts < &tasks[NR_TASKS]; ++ts) {
		if (ts->state == TASK_SLEEP || ts == thiscpu->cpu_task) {
			ts->remind_ticks--;
			if (ts->remind_ticks <= 0)
				ts->state = TASK_RUNNABLE;
		} 
	}
	if (thiscpu->cpu_task->state == TASK_RUNNABLE) {
		sched_yield();
	}
}

unsigned long get_tick()
{
	return jiffies;
}

void timer_init()
{
	set_timer(TIME_HZ);

	/* Enable interrupt */
	irq_setmask_8259A(irq_mask_8259A & ~(1<<IRQ_TIMER));
}

