.globl do_syscall
do_syscall:
	pushl	%ebx
	movl	8(%esp), %eax
	movl	8(%esp), %ebx
	movl	12(%esp), %ecx
	movl	16(%esp), %edx
	int	$0x80
	popl	%ebx
	ret
