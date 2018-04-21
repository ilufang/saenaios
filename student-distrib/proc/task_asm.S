.globl	task_current_pid
.globl  task_kernel_process
.globl  task_kernel_process_length
.globl 	task_kernel_process_iret

USER_CS:
	.long   0x0023
USER_DS:
	.long	0x002B

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

task_kernel_process_iret:
	movl	USER_DS, %eax
	movw	%ax, %ds
	pushl 	%eax
	pushl 	0xC00000	// NOTE BRUTAL STACK HERE
	pushf
	pushl 	USER_CS
	pushl 	task_kernel_process
	iret


task_kernel_process:
	pushl $0
	pushl $0
	pushl $0
	call fork
	cmpl $0,%eax
	je task_kernel_child
	jmp task_kernel_parent


task_kernel_child:
	pushl $task_kernel_child_string
	call printf
	jmp task_kernel_child

task_kernel_parent:
	pushl $task_kernel_parent_string
	call printf
	jmp task_kernel_parent

task_kernel_process_end:

task_kernel_process_length:
	.long task_kernel_process_end - task_kernel_process

task_kernel_parent_string:
	.string "parent\n"

task_kernel_child_string:
	.string "child\n"
