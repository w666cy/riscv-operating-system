#include "os.h"

#define DELAY 1000

void user_task0(void *param)
{
	uart_puts("Task 0 is Created!		");
	while (1) {
		// uart_puts("Task 0: Running...\n");
		printf("Task %d is Running\n", param);
		task_delay(DELAY);
		task_yield();
	}
}

void user_task1(void *param)
{
	uart_puts("Task 1 is Created!		");
	while (1) {
		// uart_puts("Task 1: Running...\n");
		printf("Task %d is Running\n", param);
		task_delay(DELAY);
		task_yield();
	}
}

void user_task2(void *param)
{
	printf("Task %d is Created!		", param);
	while (1) {
		printf("Task %d is Running\n", param);
		task_delay(DELAY);
		task_yield();
	}
}

void user_task3(void *param)
{
	printf("Task %d is Created!		", param);
	while (1) {
		printf("Task %d is Running\n", param);
		task_delay(DELAY);
		task_yield();
	}
}

void user_task4(void *param)
{
	printf("Task %d is Created!		", param);
	while (1) {
		printf("Task %d is Running\n", param);
		task_delay(DELAY);
		task_yield();
	}
}

void user_task5(void *param)
{
	printf("Task %d is Created!		", param);
	while (1) {
		printf("Task %d is Running\n", param);
		task_delay(DELAY);
		task_yield();
	}
}

void user_task6(void *param)
{
	printf("Task %d is Created!		", param);
	while (1) {
		printf("Task %d is Running\n", param);
		task_delay(DELAY);
		// task_yield();
		task_exit();
	}
}

/* NOTICE: DON'T LOOP INFINITELY IN main() */
void os_main(void)
{
	int task_id0 = 0;
    int task_id1 = 1;
    int task_id2 = 2;
    int task_id3 = 3;
    int task_id4 = 4;
    int task_id5 = 5;
    int task_id6 = 6;

    task_create(user_task0, &task_id0, 10);
    task_create(user_task1, &task_id1, 6);
    task_create(user_task2, &task_id2, 5);
    task_create(user_task3, &task_id3, 4);
    task_create(user_task4, &task_id4, 3);
    task_create(user_task5, &task_id5, 2);
    task_create(user_task6, &task_id6, 1);
}

