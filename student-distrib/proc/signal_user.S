.globl signal_user_base
.globl signal_user_length

.globl signal_user_ret
.globl signal_user_ret_offset

.globl  task_kernel_process
.globl 	task_kernel_process_iret



signal_user_base:
	.long 0xecebcafe // Dummy

signal_user_ret:
	addl	$4, %esp
	popfl
	popal
	ret

USER_CS:
	.long   0x0023
USER_DS:
	.long	0x002B

task_kernel_process_iret:
	movl	USER_DS, %eax
	movw	%ax, %ds
	pushl 	%eax
	pushl 	$0x8001000	// NOTE BRUTAL STACK HERE
	pushf
	pushl 	USER_CS
	pushl 	$task_kernel_process
	iret


task_kernel_process:
	call fork
	cmpl $0,%eax
	je task_kernel_child
	jmp task_kernel_parent


task_kernel_child:
	call task_kernel_get_eip

task_kernel_child_offset_flag:
	movl task_kernel_child_string_offset(%eax), %eax
	pushl %eax
	call printf
	add $4, %esp
	jmp task_kernel_child

task_kernel_parent:
	call task_kernel_get_eip

task_kernel_parent_offset_flag:
	movl  task_kernel_parent_string_offset(%eax), %eax
	pushl %eax
	call printf
	add $4, %esp
	jmp task_kernel_parent


task_kernel_parent_string:
	.string "parent\n"

task_kernel_parent_string_offset:
	.long task_kernel_parent_string - task_kernel_parent_offset_flag

task_kernel_child_string:
	.string "child\n"

task_kernel_child_string_offset:
	.long task_kernel_child_string - task_kernel_child_offset_flag

task_kernel_get_eip:
	movl 	4(%esp), %eax
	ret

signal_user_length:
	.long	signal_user_length - signal_user_base

signal_user_ret_offset:
	.long	signal_user_ret - signal_user_base
