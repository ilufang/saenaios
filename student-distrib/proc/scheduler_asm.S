#define STACK_UNKO_MAGIC $1145141919

.global scheduler_get_magic
.global scheduler_iret
//.global scheduler_kernel_iret

scheduler_get_magic:
	pushl	%ebp
	movl	%esp, %ebp

	movl	%esp, %ecx

scheduler_get_magic_loop:
	movl	(%ecx),	%eax
	cmpl	STACK_UNKO_MAGIC, %eax
	je		scheduler_get_magic_found
	addl	$4, %ecx
	jmp		scheduler_get_magic_loop

scheduler_get_magic_found:
	leal	(%ecx), %eax
	leave
	ret

scheduler_static_space:
	.long	0

scheduler_iret:
	// need return addr, cs, eflags, esp, ss
	// parameter is the address of the structure of regs
	// in order of edi esi ebp esp ebx edx ecx eax

	movl	4(%esp), %esp
	movl	%esp, %eax
	addl	$36, %eax
	movl	%eax, scheduler_static_space

	addl	$4, %esp

	popal
	// normalize esp from previously saved space
	movl	scheduler_static_space, %esp

	iret

/* scheduler_kernel_iret:
	movl	8(%esp), %eax
	movl	%eax, scheduler_static_space

	movl	4(%esp), %esp
	addl	$4, %esp

	popal

	movl	scheduler_static_space, %esp

	iret
 */
