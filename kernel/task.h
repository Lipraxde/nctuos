#ifndef TASK_H
#define TASK_H

#include <inc/trap.h>
#include <kernel/mem.h>
#define NR_TASKS	10
#define TIME_QUANT	100

typedef enum
{
	TASK_FREE = 0,
	TASK_RUNNABLE,
	TASK_RUNNING,
	TASK_SLEEP,
	TASK_STOP,
} TaskState;

struct Task
{
	int task_id;
	int parent_id;
	struct Trapframe tf;	// Saved registers
	int32_t remind_ticks;
	TaskState state;	// Task state
	pde_t *pgdir;		// Per process Page Directory
	struct Task *task_link;	// next free or next task...
};

void task_init();
void env_pop_tf(struct Trapframe *tf);

void sys_kill(int pid);
int sys_fork(void);

struct Task *cur_task;

#endif
