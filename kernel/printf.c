// Simple implementation of cprintf console output for the kernel,
// based on printfmt() and the kernel console's cputchar().
#include <inc/types.h>
#include <inc/stdio.h>

// kernel/screen.c
extern void putch(unsigned char c);

// kernel/kbd.c
extern int getc(void);

static void
_putch(int ch, int *cnt)
{
	putch(ch);
	(*cnt)++;
}

int
vcprintf(const char *fmt, va_list ap)
{
	int cnt = 0;

	vprintfmt((void*)_putch, &cnt, fmt, ap);
	return cnt;
}

int
cprintf(const char *fmt, ...)
{
	va_list ap;
	int cnt;

	va_start(ap, fmt);
	cnt = vcprintf(fmt, ap);
	va_end(ap);

	return cnt;
}

