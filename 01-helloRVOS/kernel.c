extern void uart_init(void);
extern void uart_puts(char *s);

void start_kernel(void)
{
	uart_init();
	uart_puts("Hello, RVOS!\n");

	// while (1) {}; // stop here!
	while (1) {
        char ch = uart_getc();
		if (ch == '\r')
			uart_puts("\n");
        uart_puts(&ch);
    }
}

