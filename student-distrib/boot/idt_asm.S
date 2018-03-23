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

idt_int_de:
	pushl	(%esp)
	pushl	$idt_int_msg_de
	call	idt_int_panic
	addl	$8, %esp
	iret
idt_int_nmi:
	pushl	$idt_int_msg_nmi
	call	idt_int_panic
	addl	$4, %esp
	iret
idt_int_bp:
	call	idt_int_bp_handler
	iret
idt_int_of:
	call	idt_int_of_handler
	iret
idt_int_br:
	pushl	(%esp)
	pushl	$idt_int_msg_br
	call	idt_int_panic
	addl	$8, %esp
	iret
idt_int_ud:
	pushl	(%esp)
	pushl	$idt_int_msg_ud
	call	idt_int_panic
	addl	$8, %esp
	iret
idt_int_nm:
	pushl	(%esp)
	pushl	$idt_int_msg_nm
	call	idt_int_panic
	addl	$8, %esp
	iret
idt_int_df:
	pushl	$idt_int_msg_df
	call	idt_int_panic
	addl	$4, %esp
	iret
idt_int_ts:
	pushl	(%esp)	// Err code
	pushl	8(%esp)	// EIP
	pushl	$idt_int_msg_ts
	call	idt_int_panic
	addl	$16, %esp // 3 args + 1 err code
	iret
idt_int_np:
	pushl	(%esp)	// Err code
	pushl	8(%esp)	// EIP
	pushl	$idt_int_msg_np
	call	idt_int_panic
	addl	$16, %esp // 3 args + 1 err code
	iret
idt_int_ss:
	pushl	(%esp)	// Err code
	pushl	8(%esp)	// EIP
	pushl	$idt_int_msg_ss
	call	idt_int_panic
	addl	$16, %esp // 3 args + 1 err code
	iret
idt_int_gp:
	pushl	(%esp)	// Err code
	pushl	8(%esp)	// EIP
	pushl	$idt_int_msg_gp
	call	idt_int_panic
	addl	$16, %esp // 3 args + 1 err code
	iret
idt_int_pf:
	movl	%cr2, %eax
	pushl	%eax	// Faulting address
	movl	4(%esp), %eax
	andl	$0xf, %eax
	pushl	%eax	// Err code (masked)
	pushl	12(%esp)// EIP
	pushl	$idt_int_msg_pf
	call	idt_int_panic
	addl	$20, %esp // 4 args + 1 err code
	iret
idt_int_mf:
	pushl	(%esp)	// Err code
	pushl	8(%esp)	// EIP
	pushl	$idt_int_msg_mf
	call	idt_int_panic
	addl	$16, %esp // 3 args + 1 err code
	iret
idt_int_ac:
	pushl	(%esp)	// Err code
	pushl	8(%esp)	// EIP
	pushl	$idt_int_msg_ac
	call	idt_int_panic
	addl	$8, %esp // 1 arg + 1 err code
	iret
idt_int_mc:
	pushl	$idt_int_msg_mc
	call	idt_int_panic
	addl	$4, %esp
	iret
idt_int_xf:
	pushl	(%esp)	// Err code
	pushl	8(%esp)	// EIP
	pushl	$idt_int_msg_xf
	call	idt_int_panic
	addl	$8, %esp // 1 arg + 1 err code
	iret
idt_int_reserved:
	pushl	$idt_int_msg_reserved
	call	idt_int_panic
	addl	$4, %esp
	iret

.align 4
idt_int_irq_listeners:
	.rept	16
	.long	idt_int_irq_default
	.endr

idt_int_irq:
idt_int_irq20:
	pushl	$0
	call	*(idt_int_irq_listeners+0)
	addl	$4, %esp
	iret
idt_int_irq21:
	pushl	$1
	call	*(idt_int_irq_listeners+4)
	addl	$4, %esp
	iret
idt_int_irq22:
	pushl	$2
	call	*(idt_int_irq_listeners+8)
	addl	$4, %esp
	iret
idt_int_irq23:
	pushl	$3
	call	*(idt_int_irq_listeners+12)
	addl	$4, %esp
	iret
idt_int_irq24:
	pushl	$4
	call	*(idt_int_irq_listeners+16)
	addl	$4, %esp
	iret
idt_int_irq25:
	pushl	$5
	call	*(idt_int_irq_listeners+20)
	addl	$4, %esp
	iret
idt_int_irq26:
	pushl	$6
	call	*(idt_int_irq_listeners+24)
	addl	$4, %esp
	iret
idt_int_irq27:
	pushl	$7
	call	*(idt_int_irq_listeners+28)
	addl	$4, %esp
	iret
idt_int_irq28:
	pushl	$8
	call	*(idt_int_irq_listeners+32)
	addl	$4, %esp
	iret
idt_int_irq29:
	pushl	$9
	call	*(idt_int_irq_listeners+36)
	addl	$4, %esp
	iret
idt_int_irq2a:
	pushl	$10
	call	*(idt_int_irq_listeners+40)
	addl	$4, %esp
	iret
idt_int_irq2b:
	pushl	$11
	call	*(idt_int_irq_listeners+44)
	addl	$4, %esp
	iret
idt_int_irq2c:
	pushl	$12
	call	*(idt_int_irq_listeners+48)
	addl	$4, %esp
	iret
idt_int_irq2d:
	pushl	$13
	call	*(idt_int_irq_listeners+52)
	addl	$4, %esp
	iret
idt_int_irq2e:
	pushl	$14
	call	*(idt_int_irq_listeners+56)
	addl	$4, %esp
	iret
idt_int_irq2f:
	pushl	$15
	call	*(idt_int_irq_listeners+60)
	addl	$4, %esp
	iret
idt_int_irq_size:
	.long	idt_int_irq21 - idt_int_irq20

idt_int_usr:
	pushl	%eax
	call	idt_int_usr_handler
	addl	$4, %esp
	iret
