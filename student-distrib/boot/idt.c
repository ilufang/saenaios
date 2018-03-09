#include "idt.h"

#include "idt_int.h"
#include "../lib.h"

#define IDT_DPL_KERNEL	0
#define IDT_DPL_USER	3

/**
 *	Create an IDT entry template
 */
void idt_make_entry(idt_desc_t *idte, void *handler, int dpl);

/**
 *	Create an IDT interrupt gate
 */
void idt_make_interrupt(idt_desc_t *idte, void *handler, int dpl);

/**
 *	Create an IDT trap gate
 */
void idt_make_trap(idt_desc_t *idte, void *handler, int dpl);

/**
 *	User interrupt (INT 0x80) handler
 */
void idt_int_usr();

void idt_construct(idt_desc_t *idt) {
	printf("Initializing IDT...\n");
	int i;
	idt_make_trap(idt + 0x00, &(idt_int_de), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x01, &(idt_int_reserved), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x02, &(idt_int_nmi), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x03, &(idt_int_bp), IDT_DPL_USER);
	idt_make_trap(idt + 0x04, &(idt_int_of), IDT_DPL_USER);
	idt_make_trap(idt + 0x05, &(idt_int_br), IDT_DPL_KERNEL);
	idt_make_trap(idt + 0x06, &(idt_int_ud), IDT_DPL_KERNEL);
	idt_make_trap(idt + 0x07, &(idt_int_nm), IDT_DPL_KERNEL);
	idt_make_trap(idt + 0x08, &(idt_int_df), IDT_DPL_KERNEL);
	idt_make_trap(idt + 0x09, &(idt_int_reserved), IDT_DPL_KERNEL);
	idt_make_trap(idt + 0x0a, &(idt_int_ts), IDT_DPL_KERNEL);
	idt_make_trap(idt + 0x0b, &(idt_int_np), IDT_DPL_KERNEL);
	idt_make_trap(idt + 0x0c, &(idt_int_ss), IDT_DPL_KERNEL);
	idt_make_trap(idt + 0x0d, &(idt_int_gp), IDT_DPL_KERNEL);
	idt_make_interrupt(idt + 0x0e, &(idt_int_pf), IDT_DPL_KERNEL);
	idt_make_trap(idt + 0x0f, &(idt_int_reserved), IDT_DPL_KERNEL);
	idt_make_trap(idt + 0x10, &(idt_int_mf), IDT_DPL_KERNEL);
	idt_make_trap(idt + 0x11, &(idt_int_ac), IDT_DPL_KERNEL);
	idt_make_trap(idt + 0x12, &(idt_int_mc), IDT_DPL_KERNEL);
	idt_make_trap(idt + 0x13, &(idt_int_xf), IDT_DPL_KERNEL);
	for (i = 0x14; i < 0x20; i++) {
		idt_make_trap(idt + i, &(idt_int_reserved), IDT_DPL_KERNEL);
	}

	idt_make_trap(idt + 0x80, &(idt_int_usr), IDT_DPL_USER);
    idt_make_interrupt(idt + RTC_VEC, &(idt_int_rtc), IDT_DPL_KERNEL);
    idt_make_interrupt(idt + KEYBOARD_VEC, &(idt_int_keyboard), IDT_DPL_KERNEL);
}

void idt_make_entry(idt_desc_t *idte, void *handler, int dpl) {
	uint32_t addr32 = (uint32_t) handler;
	(*idte).val[0] = addr32 & 0x0000ffff;
	(*idte).val[1] = addr32 & 0xffff0000;
	(*idte).seg_selector = 2 * 8; // Kernel Code Segment
	(*idte).present = 1;
	(*idte).size = 1; // Always 32 bits
	(*idte).dpl = dpl;
}

void idt_make_interrupt(idt_desc_t *idte, void *handler, int dpl) {
	idt_make_entry(idte, handler, dpl);
	(*idte).reserved4 = 0;
	(*idte).reserved3 = 0;
	(*idte).reserved2 = 1;
	(*idte).reserved1 = 1;
	(*idte).reserved0 = 0;
}

void idt_make_trap(idt_desc_t *idte, void *handler, int dpl) {
	idt_make_entry(idte, handler, dpl);
	(*idte).reserved4 = 0;
	(*idte).reserved3 = 1;
	(*idte).reserved2 = 1;
	(*idte).reserved1 = 1;
	(*idte).reserved0 = 0;
}
