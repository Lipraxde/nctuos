#include <kernel/cpu.h>
#include <kernel/task.h>
#include <kernel/timer.h>
#include <kernel/spinlock.h>
#include <inc/x86.h>

#define ctx_switch(ts) \
  do { task_pop_tf(&((ts)->tf)); } while(0)

extern bool booted;

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
	int i, j;
	long jiffies = get_tick();

	if (!booted)
		task_pop_tf(thiscpu->last_tf);

	// start the first task
	if (thiscpu->cpu_task == 0) {
		thiscpu->cpu_task = &tasks[cpunum()];
		lcr3(PADDR(tasks[cpunum()].pgdir));
		task_pop_tf(&thiscpu->cpu_task->tf);
	}

	spin_lock(&tasks_lock);
	// Wake up tasks
	if (cpunum() == 0) {
		struct Task *ts;
		for (ts = &tasks[ncpu]; ts < &tasks[NR_TASKS]; ++ts) {
			if (ts->state == TASK_SLEEP) {
				if (ts->pick_tick - jiffies <= 0)
					ts->state = TASK_RUNNABLE;
			}
		}
	}

	if (thiscpu->cpu_task->state == TASK_STOP)
		task_free(thiscpu->cpu_task->task_id);

	// Test task should be preempted?
	if (thiscpu->cpu_task->state == TASK_RUNNING) {
		if ((thiscpu->cpu_task->pick_tick - jiffies <= 0) || (thiscpu->cpu_task->task_id < ncpu))
			thiscpu->cpu_task->state = TASK_RUNNABLE;
		else
			ctx_switch(thiscpu->cpu_task);
	}

	// Find current task
	// My implement 'task_id = i = &tasks[i] - &tasks[0]'
	i = thiscpu->cpu_task->task_id;

	// Skip idle task
	if (i < ncpu)
		i = ncpu + cpunum();

	// Search from the next task
	j = i;
	do {
		i += ncpu;
		if (i >= NR_TASKS)
			i = i - NR_TASKS + ncpu;
	} while ((tasks[i].state != TASK_RUNNABLE) && (i != j));

	// No runnable task is found, select idle task
	if (tasks[i].state != TASK_RUNNABLE)
		i = cpunum();

	// Assert task start form runnable state
	assert(tasks[i].state == TASK_RUNNABLE);

	thiscpu->cpu_task = &tasks[i];
	thiscpu->cpu_task->state = TASK_RUNNING;
	thiscpu->cpu_task->pick_tick = get_tick() + (i < ncpu) ? 0 : TIME_QUANT;
	spin_unlock(&tasks_lock);
	lcr3(PADDR(thiscpu->cpu_task->pgdir));
	ctx_switch(thiscpu->cpu_task);
}
