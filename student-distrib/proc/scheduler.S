#include"scheduler.h"

.global scheduler_get_magic
.global scheduler_iret

scheduler_get_magic:
	pushl	%ebp
	movl	%esp, %ebp

	movl	%esp, %ecx

scheduler_get_magic_loop:
	movl	(%ecx),	%eax
	cmpl	STACK_UNKO_MAGIC, %eax
	je		scheduler_get_magic_found
	addl	$4, %ecx

scheduler_get_magic_found:
	movl	%ecx+4, %eax

	leave
	ret

scheduler_static_space:
	.long	0

scheduler_iret:
	// need return addr, cs, eflags, esp, ss
	// parameter is the address of the structure of regs
	// in order of edi esi ebp esp ebx edx ecx eax

	movl	4(%esp), %eax
	movl 	%eax+36, ($scheduler_static_space)

	addl	$4, %esp

	popal
	// normalize esp from previously saved space
	movl	($scheduler_static_space), %esp

	iret

	iret
