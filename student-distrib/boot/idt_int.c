#include "idt_int.h"

#include "../lib.h"

void idt_int_bp_handler(int eax) {
}

void idt_int_of_handler(int eax) {

}

void idt_int_panic(char *msg) {
	printf("[CAT] Received fatal exception: \n");
	printf(msg);

	while(1) {
		asm volatile ("hlt");
	}
}

void idt_int_usr_handler(int eax) {
	printf("User Interrupt: %d\n", eax);
}
