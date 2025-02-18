#include "os.h"

/* interval ~= 1s */
#define TIMER_INTERVAL CLINT_TIMEBASE_FREQ

static uint32_t _tick = 0;

/* 定义时间变量 */
static int seconds = 0;
static int minutes = 0;
static int hours = 0;

/* load timer interval(in ticks) for next timer interrupt.*/
void timer_load(int interval)
{
	/* each CPU has a separate source of timer interrupts. */
	int id = r_mhartid();
	
	*(uint64_t*)CLINT_MTIMECMP(id) = *(uint64_t*)CLINT_MTIME + interval;
}

void timer_init()
{
	/*
	 * On reset, mtime is cleared to zero, but the mtimecmp registers 
	 * are not reset. So we have to init the mtimecmp manually.
	 */
	timer_load(TIMER_INTERVAL);

	/* enable machine-mode timer interrupts. */
	w_mie(r_mie() | MIE_MTIE);

	/* enable machine-mode global interrupts. */
	w_mstatus(r_mstatus() | MSTATUS_MIE);

	printf("00:00:00");
}

void timer_handler() 
{
	_tick++;
	// printf("tick: %d\n", _tick);

	/* 更新时间 */
    seconds++;
    if (seconds >= 60) {
        seconds = 0;
        minutes++;
        if (minutes >= 60) {
            minutes = 0;
            hours++;
            if (hours >= 24) {
                hours = 0;
            }
        }
    }

    /* 使用回车符覆盖上一秒的时间 */
    uart_puts("\r");

    /* 手动格式化输出时间 */
    uart_putc('0' + (hours / 10));
    uart_putc('0' + (hours % 10));
    uart_putc(':');
    uart_putc('0' + (minutes / 10));
    uart_putc('0' + (minutes % 10));
    uart_putc(':');
    uart_putc('0' + (seconds / 10));
    uart_putc('0' + (seconds % 10));

	timer_load(TIMER_INTERVAL);
}
