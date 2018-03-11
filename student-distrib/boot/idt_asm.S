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
.globl idt_int_usr

.globl idt_int_rtc
.globl idt_int_keyboard

idt_int_rtc:
    pushal
    call    rtc_handler
    popal
    iret

idt_int_keyboard:
    pushal
    call    keyboard_handler
    popal
    iret

idt_int_msg_de:
	.string "Divide Error at %x"
idt_int_msg_nmi:
	.string "NMI"
idt_int_msg_bp:
	.string "Breakpoint"
idt_int_msg_of:
	.string "Overflow"
idt_int_msg_br:
	.string "Bound Range Exceeded at %x"
idt_int_msg_ud:
	.string "Undefined Opcode at %x"
idt_int_msg_nm:
	.string "No Math Coprocessor at %x"
idt_int_msg_df:
	.string "Double Fault"
idt_int_msg_ts:
	.string "Invalid TSS at %x (err=%x)"
idt_int_msg_np:
	.string "Segment Not Present at %x (err=%x)"
idt_int_msg_ss:
	.string "Stack-Segment Fault at %x (err=%x)"
idt_int_msg_gp:
	.string "General Protection Fault at %x (err=%x)"
idt_int_msg_pf:
	.string "Page Fault at %x (err=%x, addr=%x)"
idt_int_msg_mf:
	.string "Math Fault at %x"
idt_int_msg_ac:
	.string "Alignment Check at %x"
idt_int_msg_mc:
	.string "Machine Check"
idt_int_msg_xf:
	.string "SIMD Floating Point Exception at %x"
idt_int_msg_reserved:
	.string "Reserved Interrupt"


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

idt_int_usr:
	pushl	%eax
	call	idt_int_usr_handler
	addl	$4, %esp
	iret

