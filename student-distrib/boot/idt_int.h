/**
*	@file boot/idt_int.h
 *	@brief Interrupt handlers
 *
 *	This file defines the interrupt handlers and related functions to handle
 *	interrupts from the processor. These functions are used in the IDT.
 *
 *	Due to the nature of interrupt handler, each handler is defined as two
 *	labels: an entry point and an actual handler. The entry point is implemented
 *	in idt_asm.S file, which primarily handles extraction of error information
 *	and `iret`. The handler will be called by the entry point.
 */

#ifndef BOOT_IDT_INT_H
#define BOOT_IDT_INT_H

#include "../keyboard.h"
#include "../rtc.h"

/**
 *	Divide Error entry point
 *
 *	@note This is an entry label that handles interrupt handler setup and
 *		  teardown.
 */
void idt_int_de();

/**
 *	NMI entry point
 *
 *	@note This is an entry label that handles interrupt handler setup and
 *		  teardown.
 */
void idt_int_nmi();

/**
 *	Breakpoint entry point
 *
 *	@note This is an entry label that handles interrupt handler setup and
 *		  teardown.
 */
void idt_int_bp();

/**
 *	Overflow entry point
 *
 *	@note This is an entry label that handles interrupt handler setup and
 *		  teardown.
 */
void idt_int_of();

/**
 *	Bound Range Exceeded entry point
 *
 *	@note This is an entry label that handles interrupt handler setup and
 *		  teardown.
 */
void idt_int_br();

/**
 *	Undefined Opcode entry point
 *
 *	@note This is an entry label that handles interrupt handler setup and
 *		  teardown.
 */
void idt_int_ud();

/**
 *	No Math Coprocessor entry point
 *
 *	@note This is an entry label that handles interrupt handler setup and
 *		  teardown.
 */
void idt_int_nm();

/**
 *	Double Fault entry point
 *
 *	@note This is an entry label that handles interrupt handler setup and
 *		  teardown.
 */
void idt_int_df();

/**
 *	Invalid TSS entry point
 *
 *	@note This is an entry label that handles interrupt handler setup and
 *		  teardown.
 */
void idt_int_ts();

/**
 *	Segment Not Present entry point
 *
 *	@note This is an entry label that handles interrupt handler setup and
 *		  teardown.
 */
void idt_int_np();

/**
 *	Stack-Segment Fault entry point
 *
 *	@note This is an entry label that handles interrupt handler setup and
 *		  teardown.
 */
void idt_int_ss();

/**
 *	General Protection Fault entry point
 *
 *	@note This is an entry label that handles interrupt handler setup and
 *		  teardown.
 */
void idt_int_gp();

/**
 *	Page Fault entry point
 *
 *	@note This is an entry label that handles interrupt handler setup and
 *		  teardown.
 */
void idt_int_pf();

/**
 *	Math Fault entry point
 *
 *	@note This is an entry label that handles interrupt handler setup and
 *		  teardown.
 */
void idt_int_mf();

/**
 *	Alignment Check entry point
 *
 *	@note This is an entry label that handles interrupt handler setup and
 *		  teardown.
 */
void idt_int_ac();

/**
 *	Machine Check entry point
 *
 *	@note This is an entry label that handles interrupt handler setup and
 *		  teardown.
 */
void idt_int_mc();

/**
 *	SIMD Floating Point Exception entry point
 *
 *	@note This is an entry label that handles interrupt handler setup and
 *		  teardown.
 */
void idt_int_xf();

/**
 *	Reserved Interrupt entry point
 *
 *	@note This is an entry label that handles interrupt handler setup and
 *		  teardown.
 */
void idt_int_reserved();

/**
 *	Breakpoint handler
 *
 *	Currently does nothing
 */
void idt_int_bp_handler();

/**
 *	Overflow handler
 *
 *	Currently does nothing
 */
void idt_int_of_handler();

/**
 *	System call handler
 *
 *	This function handles `INT 0x80`. It is the entry point of all system calls.
 *
 *	@param eax: The system call index
 */
void idt_int_usr_handler(int eax);

/**
 *	Prints panic message and halts the processor.
 *
 *	@param msg: The panic message. Could be a formatted string with up to four
 *				parameters
 *	@param a,b,c,d: optional payload values for the formats in `msg`
 */
void idt_int_panic(char *msg, int a, int b, int c, int d);

void idt_int_rtc();
void idt_int_keyboard();

#endif
