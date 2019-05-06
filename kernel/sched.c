#include <kernel/cpu.h>
#include <kernel/task.h>
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
*    and set its state, remind_ticks, and change page
*    directory to its pgdir.
*
* 4. CONTEXT SWITCH, leverage the macro ctx_switch(ts)
*    Please make sure you understand the mechanism.
*/
void sched_yield(void)
{
	static int i = 0;

	if (cpunum())
		while(1);

	do {
		i = i == NR_TASKS ? 0 : i + 1;
	} while (tasks[i].state != TASK_RUNNABLE);

	thiscpu->cpu_task = &tasks[i];
	thiscpu->cpu_task->state = TASK_RUNNING;
	thiscpu->cpu_task->remind_ticks = TIME_QUANT;
	lcr3(PADDR(thiscpu->cpu_task->pgdir));
	ctx_switch(thiscpu->cpu_task);
}
