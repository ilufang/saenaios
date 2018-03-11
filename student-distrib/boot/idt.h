/**
*	@file boot/idt.h
 *	@brief IDT functionalities
 *
 *	This file provides functions to edit IDT entries, including constructing
 *	the data structure of Interrupt Gates and Trap Gates and the initialization
 *	of the entire data structure.
 *
 *	This file does not perform `lidt`. Rather, boot.S calls functions in this
 *	file to initialize the IDT data structure owned by it and executes `lidt`
 *	afterwards.
 */

#ifndef BOOT_IDT_H
#define BOOT_IDT_H

#include "../x86_desc.h"

#define KEYBOARD_VEC    0x21
#define RTC_VEC         0x28

/**
 *	Construct IDT table
 *
 *	This function will initialize the system exceptions (first 32 interrupts)
 *	and the system call handlers.
 *
 *	@param idt: Pointer to the IDT data structure. Memory for all 256 entries
 *				must be allocated.
 */
void idt_construct(idt_desc_t *idt);

#endif
