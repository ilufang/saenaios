#ifndef BOOT_IDT_INT_H
#define BOOT_IDT_INT_H

/**
 *	Divide Error Handler
 */
void idt_int_de();

/**
 *	NMI Handler
 */
void idt_int_nmi();

/**
 *	Breakpoint Handler
 */
void idt_int_bp();

/**
 *	Overflow Handler
 */
void idt_int_of();

/**
 *	Bound Range Exceeded Handler
 */
void idt_int_br();

/**
 *	Undefined Opcode Handler
 */
void idt_int_ud();

/**
 *	No Math Coprocessor Handler
 */
void idt_int_nm();

/**
 *	Double Fault Handler
 */
void idt_int_df();

/**
 *	Invalid TSS Handler
 */
void idt_int_ts();

/**
 *	Segment Not Present Handler
 */
void idt_int_np();

/**
 *	Stack-Segment Fault Handler
 */
void idt_int_ss();

/**
 *	General Protection Fault Handler
 */
void idt_int_gp();

/**
 *	Page Fault Handler
 */
void idt_int_pf();

/**
 *	Math Fault Handler
 */
void idt_int_mf();

/**
 *	Alignment Check Handler
 */
void idt_int_ac();

/**
 *	Machine Check Handler
 */
void idt_int_mc();

/**
 *	SIMD Floating Point Exception Handler
 */
void idt_int_xf();

/**
 *	Reserved Interrupt Handler
 */
void idt_int_reserved();

void idt_int_bp_handler();

void idt_int_of_handler();

void idt_int_usr_handler(int eax);

void idt_int_panic(char *msg, int a, int b, int c, int d);

#endif
