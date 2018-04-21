.globl signal_user_base
.globl signal_user_length

.globl signal_user_ret
.globl signal_user_ret_offset
.globl task_kernel_process_offset

#define LD_BASE	0x8000000

signal_user_base:
	.long 0xecebcafe // Dummy

signal_user_ret:
	addl	$4, %esp
	popfl
	popal
	ret

task_kernel_process:
	movl	$15, %eax // MAKE_INITD
	int	$0x80
	movl	$23, %eax // SYSCALL_FORK
	int	$0x80
	cmpl	$0,%eax
	je	task_kernel_process$child
	jmp	task_kernel_process$parent


task_kernel_process$child:
	// SYSCALL_EXECVE
	movl	$25, %eax
	// PATH
	movl	$task_kernel_process$execve, %ebx
	// ARGV
	movl	$task_kernel_process$argv, %ecx
	// ENVP
	movl	$0, %edx
	// CALL EXECVE
	int	$0x80
	// BAD!!!
	int	$0x1

task_kernel_process$parent:
	jmp	task_kernel_process$parent

task_kernel_process$execve:
	.string	"/shell"

task_kernel_process$argv:
	.long	task_kernel_process$execve
	.long	0

signal_user_length:
	.long	signal_user_length - signal_user_base

signal_user_ret_offset:
	.long	signal_user_ret - signal_user_base
task_kernel_process_offset:
	.long	task_kernel_process - signal_user_base
