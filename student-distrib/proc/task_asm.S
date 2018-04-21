.globl	task_current_pid

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


