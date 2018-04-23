#define STACK_REG_MAGIC $1145141919

TMPVAL:
	.long	0

.globl iret_struct

iret_struct:
	.long	0

#define SIGHUP		$1	///< terminal line hangup
#define SIGINT		$2	///< interrupt program
#define SIGQUIT		$3	///< quit program
#define SIGILL		$4	///< illegal instruction
#define SIGTRAP		$5	///< trace trap
#define SIGABRT		$6	///< abort program (formerly SIGIOT)
#define SIGEMT		$7	///< emulate instruction executed
#define SIGFPE		$8	///< floating-point exception
#define SIGKILL		$9	///< kill program
#define SIGBUS		$10	///< bus error
#define SIGSEGV		$11	///< segmentation violation
#define SIGSYS		$12	///< non-existent system call invoked
#define SIGPIPE		$13	///< write on a pipe with no reader
#define SIGALRM		$14	///< real-time timer expired
#define SIGTERM		$15	///< software termination signal
#define SIGURG		$16	///< urgent condition present on socket
#define SIGSTOP		$17	///< stop (cannot be caught or ignored)
#define SIGTSTP		$18	///< stop signal generated from
#define SIGCONT		$19	///< continue after stop
#define SIGCHLD		$20	///< child status has changed
#define SIGTTIN		$21	///< background read attempted from control terminal
#define SIGTTOU		$22	///< background write attempted to control terminal
#define SIGIO		$23	///< I/O is possible on a descriptor
#define SIGXCPU		$24	///< cpu time limit exceeded
#define SIGXFSZ		$25	///< file size limit exceeded
#define SIGVTALRM	$26	///< virtual time alarm
#define SIGPROF		$27	///< profiling timer alarm
#define SIGWINCH	$28	///< Window size change
#define SIGINFO		$29	///< status request from keyboard
#define SIGUSR1		$30	///< User defined signal 1
#define SIGUSR2		$31	///< User defined signal 2
#define SIG_MAX		$32	///< Total number of signals

.globl idt_int_de
.globl idt_int_nmi
.globl idt_int_bp
.globl idt_int_of
.globl idt_int_br
.globl idt_int_ud
.globl idt_int_nm
.globl idt_int_df
.globl idt_int_ts
.globl idt_int_np
.globl idt_int_ss
.globl idt_int_gp
.globl idt_int_pf
.globl idt_int_mf
.globl idt_int_ac
.globl idt_int_mc
.globl idt_int_xf
.globl idt_int_reserved
.globl idt_int_irq0
.globl idt_int_irq1
.globl idt_int_irq2
.globl idt_int_irq3
.globl idt_int_irq4
.globl idt_int_irq5
.globl idt_int_irq6
.globl idt_int_irq7
.globl idt_int_irq8
.globl idt_int_irq9
.globl idt_int_irq10
.globl idt_int_irq11
.globl idt_int_irq12
.globl idt_int_irq13
.globl idt_int_irq14
.globl idt_int_irq15
.globl idt_int_irq_listeners
.globl idt_int_usr

.globl idt_int_rtc
.globl idt_int_keyboard

idt_int_msg_de:
	.string "Divide Error at %x\n"
idt_int_msg_nmi:
	.string "NMI\n"
idt_int_msg_bp:
	.string "Breakpoint\n"
idt_int_msg_of:
	.string "Overflow\n"
idt_int_msg_br:
	.string "Bound Range Exceeded at %x\n"
idt_int_msg_ud:
	.string "Undefined Opcode at %x\n"
idt_int_msg_nm:
	.string "No Math Coprocessor at %x\n"
idt_int_msg_df:
	.string "Double Fault\n"
idt_int_msg_ts:
	.string "Invalid TSS at %x (err=%x)\n"
idt_int_msg_np:
	.string "Segment Not Present at %x (err=%x)\n"
idt_int_msg_ss:
	.string "Stack-Segment Fault at %x (err=%x)\n"
idt_int_msg_gp:
	.string "General Protection Fault at %x (err=%x)\n"
idt_int_msg_mf:
	.string "Math Fault at %x\n"
idt_int_msg_ac:
	.string "Alignment Check at %x\n"
idt_int_msg_mc:
	.string "Machine Check\n"
idt_int_msg_xf:
	.string "SIMD Floating Point Exception at %x\n"
idt_int_msg_reserved:
	.string "Reserved Interrupt\n"

/*
 *	Regarding magical constants used in the calculation of relative stack
 *	addresses
 *
 *	On each call to the interrupt gate, the processor would push a structure
 *	used by `iret` and a dword error code in some exceptions. All gates are
 *	designed to first pop off the error code into a temporary variable, then
 *	execute a `pushal` to put all GPRs right on top of the iret structure,
 *	and then push a magical constant for code to find the saved state. In
 *	case the faulting address is needed, it would be offset 36 from the top
 *	of the saved state structure (1 magic dword and 8 saved registers).
 *	`TMPVAL` is used to save the error code.
 *
 *	When returning, all the arguments pushed for the function call, plus the
 *	magic dword, will be popped off the stack before executing `popal` and
 *	`iret`. In short, stack spaced allocated by all the visible `pushl`s are
 *	popped.
 */

idt_int_de:
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	SIGFPE

	pushl	iret_struct
	call	scheduler_update_taskregs
	addl	$4, %esp

	call	idt_int_signal
	addl	$8, %esp
	popal
	iret
idt_int_nmi:
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	$idt_int_msg_nmi

	pushl	iret_struct
	call	scheduler_update_taskregs
	addl	$4, %esp

	call	idt_int_panic
	addl	$8, %esp
	popal
	iret
idt_int_bp:
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct

	pushl	iret_struct
	call	scheduler_update_taskregs
	addl	$4, %esp

	call	idt_int_bp_handler
	addl	$4, %esp
	popal
	iret
idt_int_of:
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct

	pushl	iret_struct
	call	scheduler_update_taskregs
	addl	$4, %esp

	call	idt_int_of_handler
	addl	$4, %esp
	popal
	iret
idt_int_br:
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	SIGSEGV

	pushl	iret_struct
	call	scheduler_update_taskregs
	addl	$4, %esp

	call	idt_int_signal
	addl	$8, %esp
	popal
	iret
idt_int_ud:
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	SIGILL

	pushl	iret_struct
	call	scheduler_update_taskregs
	addl	$4, %esp

	call	idt_int_signal
	addl	$8, %esp
	popal
	iret
idt_int_nm:
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	SIGFPE

	pushl	iret_struct
	call	scheduler_update_taskregs
	addl	$4, %esp

	call	idt_int_signal
	addl	$8, %esp
	popal
	iret
idt_int_df:
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	$idt_int_msg_df

	pushl	iret_struct
	call	scheduler_update_taskregs
	addl	$4, %esp

	call	idt_int_panic
	addl	$8, %esp
	popal
	iret
idt_int_ts:
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	SIGBUS

	pushl	iret_struct
	call	scheduler_update_taskregs
	addl	$4, %esp

	call	idt_int_signal
	addl	$8, %esp
	popal
	iret
idt_int_np:
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	SIGBUS

	pushl	iret_struct
	call	scheduler_update_taskregs
	addl	$4, %esp

	call	idt_int_signal
	addl	$8, %esp
	popal
	iret
idt_int_ss:
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	SIGBUS

	pushl	iret_struct
	call	scheduler_update_taskregs
	addl	$4, %esp

	call	idt_int_signal
	addl	$8, %esp
	popal
	iret
idt_int_gp:
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	SIGSEGV

	pushl	iret_struct
	call	scheduler_update_taskregs
	addl	$4, %esp

	call	idt_int_signal
	addl	$8, %esp
	popal
	iret
idt_int_pf:
	popl	TMPVAL
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	movl	%cr2, %eax
	pushl	%eax		// Faulting address
	movl	TMPVAL, %eax
	andl	$0xf, %eax
	pushl	%eax		// Err code (masked)
	pushl	44(%esp)	// EIP

	pushl	iret_struct
	call	scheduler_update_taskregs
	addl	$4, %esp

	call	idt_int_pf_handler
	addl	$16, %esp
	popal
	iret
idt_int_mf:
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	SIGFPE

	pushl	iret_struct
	call	scheduler_update_taskregs
	addl	$4, %esp

	call	idt_int_signal
	addl	$8, %esp
	popal
	iret
idt_int_ac:
	popl	TMPVAL
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	TMPVAL		// Err code
	pushl	40(%esp)	// EIP
	pushl	$idt_int_msg_ac

	pushl	iret_struct
	call	scheduler_update_taskregs
	addl	$4, %esp

	call	idt_int_panic
	addl	$16, %esp
	popal
	iret
idt_int_mc:
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	$idt_int_msg_mc

	pushl	iret_struct
	call	scheduler_update_taskregs
	addl	$4, %esp

	call	idt_int_panic
	addl	$8, %esp
	popal
	iret
idt_int_xf:
	popl	TMPVAL
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	TMPVAL		// Err code
	pushl	40(%esp)	// EIP
	pushl	$idt_int_msg_xf

	pushl	iret_struct
	call	scheduler_update_taskregs
	addl	$4, %esp

	call	idt_int_panic
	addl	$16, %esp
	popal
	iret
idt_int_reserved:
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	$idt_int_msg_reserved

	pushl	iret_struct
	call	scheduler_update_taskregs
	addl	$4, %esp

	call	idt_int_panic
	addl	$8, %esp
	popal
	iret

idt_int_irq_listeners:
	.rept	16
	.long	idt_int_irq_default
	.endr

idt_int_irq0:
	movl	$0, TMPVAL
	jmp	idt_int_irqexec
idt_int_irq1:
	movl	$1, TMPVAL
	jmp	idt_int_irqexec
idt_int_irq2:
	movl	$2, TMPVAL
	jmp	idt_int_irqexec
idt_int_irq3:
	movl	$3, TMPVAL
	jmp	idt_int_irqexec
idt_int_irq4:
	movl	$4, TMPVAL
	jmp	idt_int_irqexec
idt_int_irq5:
	movl	$5, TMPVAL
	jmp	idt_int_irqexec
idt_int_irq6:
	movl	$6, TMPVAL
	jmp	idt_int_irqexec
idt_int_irq7:
	movl	$7, TMPVAL
	jmp	idt_int_irqexec
idt_int_irq8:
	movl	$8, TMPVAL
	jmp	idt_int_irqexec
idt_int_irq9:
	movl	$9, TMPVAL
	jmp	idt_int_irqexec
idt_int_irq10:
	movl	$10, TMPVAL
	jmp	idt_int_irqexec
idt_int_irq11:
	movl	$11, TMPVAL
	jmp	idt_int_irqexec
idt_int_irq12:
	movl	$12, TMPVAL
	jmp	idt_int_irqexec
idt_int_irq13:
	movl	$13, TMPVAL
	jmp	idt_int_irqexec
idt_int_irq14:
	movl	$14, TMPVAL
	jmp	idt_int_irqexec
idt_int_irq15:
	movl	$15, TMPVAL
	jmp	idt_int_irqexec

idt_int_irqexec:
	pushal
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct

	pushl	iret_struct
	call	scheduler_update_taskregs
	addl	$4, %esp

	movl	TMPVAL, %eax
	pushl	%eax
	call	*idt_int_irq_listeners(,%eax,4)

	addl	$8, %esp
	popal
	iret

idt_int_usr:
	pushal
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct

	pushl	%edx
	pushl	%ecx
	pushl	%ebx
	pushl	%eax

	pushl	iret_struct
	call	scheduler_update_taskregs
	addl	$4, %esp

	call	syscall_invoke
	addl	$16, %esp

	movl	%eax, TMPVAL
	addl	$4, %esp
	popal
	movl	TMPVAL, %eax

	iret
