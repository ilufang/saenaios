.globl do_syscall
do_syscall:
	pushl	%ebx
	movl	8(%esp), %eax
	movl	12(%esp), %ebx
	movl	16(%esp), %ecx
	movl	20(%esp), %edx
	int	$0x80
	popl	%ebx
	ret
