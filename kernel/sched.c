#include <kernel/cpu.h>
#include <kernel/task.h>
#include <kernel/timer.h>
#include <inc/x86.h>

#define ctx_switch(ts) \
  do { task_pop_tf(&((ts)->tf)); } while(0)

/* TODO: Lab5
* Implement a simple round-robin scheduler (Start with the next one)
*
* 1. You have to remember the task you picked last time.
*
* 2. If the next task is in TASK_RUNNABLE state, choose
*    it.
*
* 3. After your choice set cur_task to the picked task
*    and set its state, pick_tick, and change page
*    directory to its pgdir.
*
* 4. CONTEXT SWITCH, leverage the macro ctx_switch(ts)
*    Please make sure you understand the mechanism.
*/
void sched_yield(void)
{
	static int i = 0;

	if (cpunum()) {
		while(1);
	}

	unsigned long jiffies = get_tick();
	if (thiscpu->cpu_task == 0)
		thiscpu->cpu_task = &tasks[cpunum()];
  		

	if (thiscpu->cpu_task->pick_tick - jiffies > 0)
		ctx_switch(thiscpu->cpu_task);
	else
		thiscpu->cpu_task->state = TASK_RUNNABLE;

	// Wake up tasks
	struct Task *ts;
	for (ts = &tasks[0]; ts < &tasks[NR_TASKS]; ++ts) {
		if (ts->state == TASK_SLEEP) {
			if (ts->pick_tick - jiffies <= 0)
				ts->state = TASK_RUNNABLE;
		} 
	}

	do {
		i = i == NR_TASKS ? 0 : i + 1;
	} while (tasks[i].state != TASK_RUNNABLE);

	thiscpu->cpu_task = &tasks[i];
	thiscpu->cpu_task->state = TASK_RUNNING;
	thiscpu->cpu_task->pick_tick = get_tick() + TIME_QUANT;
	lcr3(PADDR(thiscpu->cpu_task->pgdir));
	ctx_switch(thiscpu->cpu_task);
}
