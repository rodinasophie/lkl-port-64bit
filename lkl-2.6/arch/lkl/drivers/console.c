#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/console.h>

#include <asm/callbacks.h>


static void console_write(struct console *con, const char *str, unsigned len)
{
	lkl_nops->print(str, len);
}

static struct console stdio_console = {
	.name	= "lkl_console",
	.write	= console_write,	
	.flags	= CON_PRINTBUFFER,
	.index	= -1,
};

int __init stdio_console_init(void)
{
	register_console(&stdio_console);
	return 0;
}

late_initcall(stdio_console_init);



