.globl	task_current_pid
.globl	task_kernel_process_iret
.globl 	syscall_ece391_execute_magic

task_current_pid:
	// Mask out lower 13 bits of current stack to get current kernel stack
	// block address
	movl	$0xffffe000, %eax
	andl	%esp, %eax
	// Temporary kernel stack is below 0x800000
	cmpl	$0x800000, %eax
	jl	task_current_pid$is_kernel
	// The first dword in the block is the process pid
	movl	(%eax), %eax
	ret
task_current_pid$is_kernel:
	// The temp. kernel stack, return 0
	xorl	%eax, %eax
	ret

USER_CS:
	.long	0x0023
USER_DS:
	.long	0x002B

task_kernel_process_iret:
	movl	USER_DS, %eax
	movw	%ax, %ds
	pushl 	%eax
	pushl 	$0x8001000	// NOTE BRUTAL STACK HERE
	pushf
	pushl 	USER_CS
	movl 	task_kernel_process_offset, %eax
	addl	$0x8000000, %eax
	pushl	%eax
	iret
