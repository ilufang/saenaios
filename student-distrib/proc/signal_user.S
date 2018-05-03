.globl signal_user_base
.globl signal_user_length

.globl signal_user_ret_offset
.globl signal_user_make_syscall_offset
.globl task_kernel_process_offset
.globl syscall_ece391_execute_magic

#define LD_BASE	0x8000000

signal_user_base:
	.long 0xecebcafe // Dummy

signal_user_ret:
	// Teardown stack for handler
	addl	$4, %esp
	// Restore signal mask
	movl	$29, %eax // sigprocmask
	movl	$3, %ebx // SIG_SETMASK
	movl	%esp, %ecx // Original mask
	xorl	%edx, %edx // No oldset
	int	$0x80
	addl	$4, %esp // Teardown original mask
	// Restore previous execution status
	popfl
	addl	$4, %esp // Teardown magic
	movl	12(%esp), %eax // Get ESP_K (ECE391 workaround)
	movl	%eax, 28(%esp) // Set to EAX
	popal
	ret

signal_user_make_syscall:
	int		$0x80

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
	.string	"/login"

task_kernel_process$argv:
	.long	task_kernel_process$execve
	.long	0

task_ece391_execute:
	movl	$2, %eax

/*
	These are what used for ece391 execute
*/
syscall_ece391_execute_magic_start:
	int 	$0x80 					// eax is already loaded with execve number
	movl	$24, %eax 				// if execve returned to this line, means error occurs
	movl	$-1, %ebx
	int 	$0x80 					// then call syscall__exit

signal_user_length:
	.long	signal_user_length - signal_user_base

signal_user_ret_offset:
	.long	signal_user_ret - signal_user_base
signal_user_make_syscall_offset:
	.long	signal_user_make_syscall - signal_user_base
task_kernel_process_offset:
	.long	task_kernel_process - signal_user_base
syscall_ece391_execute_magic:
	.long  	syscall_ece391_execute_magic_start - signal_user_base
