#include "idt.h"

#include "idt_int.h"
#include "../lib.h"

/**
 *	Create an IDT entry template
 */
void idt_make_entry(idt_desc_t *idte, void *handler);

/**
 *	Create an IDT interrupt gate
 */
void idt_make_interrupt(idt_desc_t *idte, void *handler);

/**
 *	Create an IDT trap gate
 */
void idt_make_trap(idt_desc_t *idte, void *handler);

/**
 *	User interrupt (INT 0x80) handler
 */
void idt_int_usr();

void idt_construct(idt_desc_t *idt) {
	printf("Initializing IDT...\n");
	int i;
	idt_make_interrupt(idt + 0x00, &(idt_int_de));
	idt_make_interrupt(idt + 0x01, &(idt_int_reserved));
	idt_make_interrupt(idt + 0x02, &(idt_int_nmi));
	idt_make_trap(idt + 0x03, &(idt_int_bp));
	idt_make_trap(idt + 0x04, &(idt_int_of));
	idt_make_interrupt(idt + 0x05, &(idt_int_br));
	idt_make_interrupt(idt + 0x06, &(idt_int_ud));
	idt_make_interrupt(idt + 0x07, &(idt_int_nm));
	idt_make_interrupt(idt + 0x08, &(idt_int_df));
	idt_make_interrupt(idt + 0x09, &(idt_int_reserved));
	idt_make_interrupt(idt + 0x0a, &(idt_int_ts));
	idt_make_interrupt(idt + 0x0b, &(idt_int_np));
	idt_make_interrupt(idt + 0x0c, &(idt_int_ss));
	idt_make_interrupt(idt + 0x0d, &(idt_int_gp));
	idt_make_interrupt(idt + 0x0e, &(idt_int_pf));
	idt_make_interrupt(idt + 0x0f, &(idt_int_reserved));
	idt_make_interrupt(idt + 0x10, &(idt_int_mf));
	idt_make_interrupt(idt + 0x11, &(idt_int_ac));
	idt_make_interrupt(idt + 0x12, &(idt_int_mc));
	idt_make_interrupt(idt + 0x13, &(idt_int_xf));
	for (i = 0x14; i < 0x20; i++) {
		idt_make_interrupt(idt + i, &(idt_int_reserved));
	}
	idt_make_trap(idt + 0x80, &(idt_int_usr));
}

void idt_make_entry(idt_desc_t *idte, void *handler) {
	uint32_t addr32 = (uint32_t) handler;
	(*idte).val[0] = addr32 & 0x0000ffff;
	(*idte).val[1] = addr32 & 0xffff0000;
	(*idte).seg_selector = 2 * 8; // Kernel Code Segment
	(*idte).present = 1;
	(*idte).size = 1; // Always 32 bits
}

void idt_make_interrupt(idt_desc_t *idte, void *handler) {
	idt_make_entry(idte, handler);
	(*idte).dpl = 0; // Priority: kernel
	(*idte).reserved4 = 0;
	(*idte).reserved3 = 0;
	(*idte).reserved2 = 1;
	(*idte).reserved1 = 1;
	(*idte).reserved0 = 0;
}

void idt_make_trap(idt_desc_t *idte, void *handler) {
	idt_make_entry(idte, handler);
	(*idte).dpl = 3; // Priority: user
	(*idte).reserved4 = 0;
	(*idte).reserved3 = 1;
	(*idte).reserved2 = 1;
	(*idte).reserved1 = 1;
	(*idte).reserved0 = 0;
}
