#include <inc/mmu.h>
#include <inc/trap.h>

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

.text
	TRAPHANDLER_NOEC(default_trap_entry, T_DEFAULT)
	TRAPHANDLER_NOEC(kbd_trap_entry, IRQ_OFFSET+IRQ_KBD)
	TRAPHANDLER_NOEC(timer_trap_entry, IRQ_OFFSET+IRQ_TIMER)
	TRAPHANDLER_NOEC(syscall_trap_entry, T_SYSCALL)

	TRAPHANDLER(default_errtrap_entry, T_DEFAULT)
	TRAPHANDLER(pgflt_trap_entry, T_PGFLT)

.globl default_trap_handler;
_alltraps:
	pushl %ds
	pushl %es
	pushal
	/* Load the Kernel Data Segment descriptor */
	mov $(GD_KD), %ax
	mov %ax, %ds
	mov %ax, %es

	pushl %esp # Pass a pointer which points to the Trapframe as an argument to default_trap_handler()
	call default_trap_handler
