#ifndef BOOT_IDT_H
#define BOOT_IDT_H

#include "../x86_desc.h"

/**
 *	Construct IDT table
 */
void idt_construct(idt_desc_t *idt);

#endif
