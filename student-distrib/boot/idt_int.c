#include "idt_int.h"

#include "../lib.h"

void idt_int_bp_handler() {
	printf("Breakpoint\n");
}

void idt_int_of_handler() {
	printf("Overflow\n");
}

void idt_int_panic(char *msg, int a, int b, int c, int d) {
	cli();
	printf("[CAT] Received fatal exception: \n");
	printf(msg, a, b, c, d);
	printf("PLEASE REBOOT.\n");
	while(1) {
		asm volatile("hlt");
	}
}

void idt_int_irq_default(int irq) {
	printf("Warning: unhandled IRQ: %x\n", irq);
}
