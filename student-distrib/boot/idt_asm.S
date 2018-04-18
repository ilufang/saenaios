#define STACK_REG_MAGIC $1145141919

TMPVAL:
	.long	0

.globl iret_struct

iret_struct:
	.long	0


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
.globl idt_int_irq
.globl idt_int_irq_size
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
idt_int_msg_pf:
	.string "Page Fault at %x (err=%x, addr=%x)\n"
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

.align 4
idt_int_de:
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	36(%esp)
	pushl	$idt_int_msg_de
	call	idt_int_panic
	addl	$12, %esp
	popal
	iret
.align 4
idt_int_nmi:
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	$idt_int_msg_nmi
	call	idt_int_panic
	addl	$8, %esp
	popal
	iret
.align 4
idt_int_bp:
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	call	idt_int_bp_handler
	addl	$4, %esp
	popal
	iret
.align 4
idt_int_of:
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	call	idt_int_of_handler
	addl	$4, %esp
	popal
	iret
.align 4
idt_int_br:
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	36(%esp)
	pushl	$idt_int_msg_br
	call	idt_int_panic
	addl	$12, %esp
	popal
	iret
.align 4
idt_int_ud:
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	36(%esp)
	pushl	$idt_int_msg_ud
	call	idt_int_panic
	addl	$12, %esp
	popal
	iret
.align 4
idt_int_nm:
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	36(%esp)
	pushl	$idt_int_msg_nm
	call	idt_int_panic
	addl	$12, %esp
	popal
	iret
.align 4
idt_int_df:
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	$idt_int_msg_df
	call	idt_int_panic
	addl	$8, %esp
	popal
	iret
.align 4
idt_int_ts:
	popl	TMPVAL		// Err code
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	TMPVAL		// Err code
	pushl	36(%esp)	// EIP
	pushl	$idt_int_msg_ts
	call	idt_int_panic
	addl	$16, %esp
	popal
	iret
.align 4
idt_int_np:
	popl	TMPVAL
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	TMPVAL		// Err code
	pushl	36(%esp)	// EIP
	pushl	$idt_int_msg_np
	call	idt_int_panic
	addl	$16, %esp
	popal
	iret
.align 4
idt_int_ss:
	popl	TMPVAL
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	TMPVAL		// Err code
	pushl	36(%esp)	// EIP
	pushl	$idt_int_msg_ss
	call	idt_int_panic
	addl	$16, %esp
	popal
	iret
.align 4
idt_int_gp:
	popl	TMPVAL
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	TMPVAL		// Err code
	pushl	36(%esp)	// EIP
	pushl	$idt_int_msg_gp
	call	idt_int_panic
	addl	$16, %esp
	popal
	iret
.align 4
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
	pushl	$idt_int_msg_pf
	call	idt_int_panic
	addl	$20, %esp
	popal
	iret
.align 4
idt_int_mf:
	popl	TMPVAL
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	TMPVAL		// Err code
	pushl	36(%esp)	// EIP
	pushl	$idt_int_msg_mf
	call	idt_int_panic
	addl	$16, %esp
	popal
	iret
.align 4
idt_int_ac:
	popl	TMPVAL
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	TMPVAL		// Err code
	pushl	36(%esp)	// EIP
	pushl	$idt_int_msg_ac
	call	idt_int_panic
	addl	$16, %esp
	popal
	iret
.align 4
idt_int_mc:
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	$idt_int_msg_mc
	call	idt_int_panic
	addl	$8, %esp
	popal
	iret
.align 4
idt_int_xf:
	popl	TMPVAL
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	TMPVAL		// Err code
	pushl	36(%esp)	// EIP
	pushl	$idt_int_msg_xf
	call	idt_int_panic
	addl	$16, %esp
	popal
	iret
.align 4
idt_int_reserved:
	pusha
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	pushl	$idt_int_msg_reserved
	call	idt_int_panic
	addl	$8, %esp
	popal
	iret

.align 4
idt_int_irq_listeners:
	.rept	16
	.long	idt_int_irq_default
	.endr

.align 4
idt_int_irq:
idt_int_irq20:
	movl	$0, TMPVAL
	jmp	idt_int_irqexec
.align 4
idt_int_irq21:
	movl	$1, TMPVAL
	jmp	idt_int_irqexec
.align 4
idt_int_irq22:
	movl	$2, TMPVAL
	jmp	idt_int_irqexec
.align 4
idt_int_irq23:
	movl	$3, TMPVAL
	jmp	idt_int_irqexec
.align 4
idt_int_irq24:
	movl	$4, TMPVAL
	jmp	idt_int_irqexec
.align 4
idt_int_irq25:
	movl	$5, TMPVAL
	jmp	idt_int_irqexec
.align 4
idt_int_irq26:
	movl	$6, TMPVAL
	jmp	idt_int_irqexec
.align 4
idt_int_irq27:
	movl	$7, TMPVAL
	jmp	idt_int_irqexec
.align 4
idt_int_irq28:
	movl	$8, TMPVAL
	jmp	idt_int_irqexec
.align 4
idt_int_irq29:
	movl	$9, TMPVAL
	jmp	idt_int_irqexec
.align 4
idt_int_irq2a:
	movl	$10, TMPVAL
	jmp	idt_int_irqexec
.align 4
idt_int_irq2b:
	movl	$11, TMPVAL
	jmp	idt_int_irqexec
.align 4
idt_int_irq2c:
	movl	$12, TMPVAL
	jmp	idt_int_irqexec
.align 4
idt_int_irq2d:
	movl	$13, TMPVAL
	jmp	idt_int_irqexec
.align 4
idt_int_irq2e:
	movl	$14, TMPVAL
	jmp	idt_int_irqexec
.align 4
idt_int_irq2f:
	movl	$15, TMPVAL
	jmp	idt_int_irqexec

idt_int_irqexec:
	pushal
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct
	movl	TMPVAL, %eax
	pushl	%eax
	call	*idt_int_irq_listeners(,%eax,4)

	addl	$8, %esp
	popal
	iret

idt_int_irq_size:
	.long	idt_int_irq21 - idt_int_irq20

idt_int_usr:
	pushal
	pushl	STACK_REG_MAGIC
	movl	%esp, iret_struct

	pushl	%edx
	pushl	%ecx
	pushl	%ebx
	pushl	%eax
	call	syscall_invoke
	addl	$16, %esp

	movl	%eax, TMPVAL
	addl	$4, %esp
	popal
	movl	TMPVAL, %eax

	iret
