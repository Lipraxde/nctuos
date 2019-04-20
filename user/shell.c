#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/syscall.h>

int main(void) {
	// shell();
	return 0;
}

void _start(void) {
	cprintf("123%d\n", 456);
	while(1);
	main();
}
