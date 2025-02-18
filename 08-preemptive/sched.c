#include "os.h"

/* defined in entry.S */
extern void switch_to(struct context *next);

#define MAX_TASKS 10
#define STACK_SIZE 1024
/*
 * In the standard RISC-V calling convention, the stack pointer sp
 * is always 16-byte aligned.
 */
uint8_t __attribute__((aligned(16))) task_stack[MAX_TASKS][STACK_SIZE];
struct context ctx_tasks[MAX_TASKS];
int priority_queue[MAX_TASKS];
int timeslice[MAX_TASKS];

/*
 * _top is used to mark the max available position of ctx_tasks
 * _current is used to point to the context of current task
 */
static int _top = 0;
static int _current = -1;

void sched_init()
{
	w_mscratch(0);

	/* enable machine-mode software interrupts. */
	w_mie(r_mie() | MIE_MSIE);
}

/*
 * implment a simple cycle FIFO schedular
 */
void schedule()
{
	int min = 256;
	for (int i = 0; i < _top; i++) {
		if (priority_queue[i] < min) {
			min = priority_queue[i];
			_current = i;
		}
	}
	printf("Priority: %d	", priority_queue[_current]);
	priority_queue[_current] += (priority_queue[_current] == 255) ? 0 : 1;
	struct context *next = &(ctx_tasks[_current]);
	switch_to(next);
}

int next_task_timeslice() {
	int min = 256;
	for (int i = 0; i < _top; i++) {
		if (priority_queue[i] < min) {
			min = priority_queue[i];
			_current = i;
		}
	}
	return timeslice[_current];
}

int task_create(void (*task)(void *param),
				void *param,
				uint8_t priority,
				uint32_t time) {
	if (_top < MAX_TASKS) {
		ctx_tasks[_top].sp = (reg_t) &task_stack[_top][STACK_SIZE];
		ctx_tasks[_top].pc = (reg_t) task;
		ctx_tasks[_top].a0 = *((int *)param);
		priority_queue[_top] = priority;
		timeslice[_top] = time;
		_top++;
		return 0;
	} else {
		return -1;
	}
}

/*
 * DESCRIPTION
 * 	task_yield()  causes the calling task to relinquish the CPU and a new 
 * 	task gets to run.
 */
void task_yield()
{
	/* trigger a machine-level software interrupt */
	int id = r_mhartid();
	*(uint32_t*)CLINT_MSIP(id) = 1;
}

/*
 * a very rough implementaion, just to consume the cpu
 */
void task_delay(volatile int count)
{
	count *= 50000;
	while (count--);
}

