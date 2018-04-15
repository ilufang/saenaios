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

scheduler_iret:
	// need return addr, cs, eflags, esp, ss
	// parameter is the address of the structure of regs
	// in order of edi esi ebp esp ebx edx ecx eax

	addl	$4, %esp	// ignore the return address of normal cdecl	

	// modify the DS register for process changing
	movl	$USER_DS, %eax
	movw	%ax, %ds

	popl 	%ebx
	popl	%ecx
	popl	%edx
	popl	%esi
	popl	%edi
	popl	%ebp
	popl	%eax

	iret
