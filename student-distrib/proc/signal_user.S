.globl signal_user_base
.globl signal_user_length

.globl signal_user_ret
.globl signal_user_ret_offset

signal_user_base:
	.long 0xecebcafe // Dummy

signal_user_ret:
	addl	$4, %esp
	popfl
	popal
	ret

signal_user_length:
	.long	signal_user_length - signal_user_base

signal_user_ret_offset:
	.long	signal_user_ret - signal_user_base
