#include "idt_int.h"
#include "../lib.h"
#include "ece391_syscall.h"
#include "../proc/task.h"

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
	task_t* curr_task = get_curr_task();
	if(curr_task->parent_pid>=0){
		syscall_ece391_halt(-1, 0, 0);
	}	// temporary solution for squashing user level program
	while(1) {
		asm volatile("hlt");
	}
}

void idt_int_irq_default(int irq) {
	printf("Warning: unhandled IRQ: %x\n", irq);
}
