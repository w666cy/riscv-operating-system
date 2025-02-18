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
// struct context kernel_task;
int priority_queue[MAX_TASKS];
int task_is_alloc[MAX_TASKS];

// void kernel_task()
// {
// 	schedule();
// }

/*
 * _top is used to mark the max available position of ctx_tasks
 * _current is used to point to the context of current task
 */
static int _top = 0;
static int _current = -1;

static void w_mscratch(reg_t x)
{
	asm volatile("csrw mscratch, %0" : : "r" (x));
}

void sched_init()
{
	for (int i = 0; i < MAX_TASKS; i++)
		task_is_alloc[i] = 0;

	w_mscratch(0);
}

/*
 * implment a simple cycle FIFO schedular
 */
// void schedule()
// {
// 	if (_top <= 0) {
// 		panic("Num of task should be greater than zero!");
// 		return;
// 	}

// 	_current = (_current + 1) % _top;
// 	struct context *next = &(ctx_tasks[_current]);
// 	switch_to(next);
// }

void schedule() {
	int min = 256;
	for (int i = 0; i < _top; i++) {
		if (task_is_alloc[i] && priority_queue[i] < min) {
			min = priority_queue[i];
			_current = i;
		}
	}
	printf("Priority: %d	", priority_queue[_current]);
	priority_queue[_current] += (priority_queue[_current] == 255) ? 0 : 1;
	struct context *next = &(ctx_tasks[_current]);
	switch_to(next);
}

/*
 * DESCRIPTION
 * 	Create a task.
 * 	- start_routin: task routine entry
 * RETURN VALUE
 * 	0: success
 * 	-1: if error occured
 */
// int task_create(void (*start_routin)(void))
// {
// 	if (_top < MAX_TASKS) {
// 		ctx_tasks[_top].sp = (reg_t) &task_stack[_top][STACK_SIZE];
// 		ctx_tasks[_top].ra = (reg_t) start_routin;
// 		_top++;
// 		return 0;
// 	} else {
// 		return -1;
// 	}
// }

int task_create(void (*task)(void *param),
				void *param,
				uint8_t priority) {
	int id = 0;
	for (int i = 0; i < _top; i++) {
		if (!task_is_alloc[i]) {
			ctx_tasks[id].sp = (reg_t) &task_stack[_top][STACK_SIZE];
			ctx_tasks[id].ra = (reg_t) task;
			ctx_tasks[id].a0 = *((int *)param);
			priority_queue[id] = priority;
			task_is_alloc[id] = 1;
			return 0;
		}
	}
	if (_top < MAX_TASKS) {
		ctx_tasks[_top].sp = (reg_t) &task_stack[_top][STACK_SIZE];
		ctx_tasks[_top].ra = (reg_t) task;
		ctx_tasks[_top].a0 = *((int *)param);
		priority_queue[_top] = priority;
		task_is_alloc[_top] = 1;
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
	// switch_to(kernel_task);
	schedule();
}

/*
 * a very rough implementaion, just to consume the cpu
 */
void task_delay(volatile int count)
{
	count *= 50000;
	while (count--);
}

void task_exit() {
	if (priority_queue[_current] > 8)
		task_is_alloc[_current] = 0;

	task_yield();
}
