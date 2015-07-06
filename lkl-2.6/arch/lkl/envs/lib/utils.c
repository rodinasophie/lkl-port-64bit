#include <stdarg.h>

#include <asm/lkl.h>
#include <asm/callbacks.h>

extern int vsnprintf(char *buf, __kernel_size_t size, const char *fmt, va_list args)
	__attribute__ ((format (printf, 3, 0)));

int lkl_printf(const char * fmt, ...)
{
	char *buffer;
	va_list args, copy;
	int n;

	va_start(args, fmt);
	va_copy(copy, args);
	n=vsnprintf(NULL, 0, fmt, copy);
	va_end(copy);

	if (!(buffer=lkl_nops->mem_alloc(n+1))) {
		va_end(args);
		return 0;
	}
	vsnprintf(buffer, n+1, fmt, args);
	va_end(args);

	lkl_nops->print(buffer, n);

	lkl_nops->mem_free(buffer);

	return n;
}
