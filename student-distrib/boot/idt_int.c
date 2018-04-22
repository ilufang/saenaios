#include "idt_int.h"

#include "../lib.h"
#include "../proc/task.h"

void idt_int_bp_handler() {
	printf("Breakpoint\n");
}

void idt_int_of_handler() {
	printf("Overflow\n");
}

void idt_int_pf_handler(int eip, int err, int addr) {
	// Check copy-on-write
	int ret;
	ret = task_pf_copy_on_write(addr);
	switch(ret) {
		case 0:
			// Success!
			return;
		case -ENOMEM:
			printf("Copy-on-write Attempted. But no memory is available\n");
			while (1);
		case -EFAULT:
		default:
			printf("[CAT] PAGE FAULT at %x, faulting address %x\n", eip, addr);
			if (err & 4)
				printf("User tried to ");
			else
				printf("Supervisor tried to ");
			if (err & 2)
				printf("write ");
			else
				printf("read ");
			if (err & 1)
				printf("a protected page");
			else
				printf("a non-existent page");
			if (err & 16)
				printf(" while fetching an instruction");
			printf("\n");
			while (1);
	}
}

void idt_int_panic(char *msg, int a, int b, int c, int d) {
	printf("[CAT] Received fatal exception: \n");
	printf(msg, a, b, c, d);
	printf("PLEASE REBOOT.\n");
	while(1) {
		// asm volatile("hlt");
	}
}

void idt_int_irq_default(int irq) {
	printf("Warning: unhandled IRQ: %x\n", irq);
}
