/**
 *	@file boot/idt.h
 *
 *	IDT functionalities
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

#define KEYBOARD_VEC    0x21			///< keyboard interrupt vector in idt table 
#define RTC_VEC         0x28			///< rtc interrupt vector in idt table

/**
 *	Construct IDT table.
 *
 *	This function will initialize the system exceptions (first 32 interrupts)
 *	and the system call handlers.
 *
 *	@param idt: Pointer to the IDT data structure. Memory for all 256 entries
 *				must be allocated.
 */
void idt_construct(idt_desc_t *idt);

/**
 *	An IRQ listener
 *
 *	Takes no argument and returns nothing
 */
typedef void (*irq_listener)();

/**
 *	Set IRQ handler for a given IRQ number
 *
 *	@param type: IRQ number to listen
 *	@param listener: pointer to handler
 *	@return 0 on success. -1 if the IRQ is already handled by another listener
 *	@note This call will also enable incoming IRQ from the specified IRQ number
 */
int idt_addEventListener(unsigned type, irq_listener listener);

/**
 *	Remove IRQ handler for the given IRQ number
 *
 *	@param type: IRQ number to unlisten
 *	@return 0 on success. -1 if the IRQ is not being listened
 *	@note This call will also disable incoming IRQ from the specified IRQ number
 */
int idt_removeEventListener(unsigned type);

/**
 *	Retrieve the current IRQ listener for the given IRQ number
 *
 *	@param type: IRQ number
 *	@return The address of the currently registered handler function, or NULL if
 *			the IRQ is not being listened to.
 */
irq_listener idt_getEventListener(unsigned type);

#endif
