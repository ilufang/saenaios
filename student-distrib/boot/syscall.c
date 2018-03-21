#include "syscall.h"

#define SYSCALL_NUMBER_MAX 256

syscall_handler syscall_handler_table[SYSCALL_NUMBER_MAX];

int syscall_register(int index, syscall_handler handler) {
	if (index < 0 || index > SYSCALL_NUMBER_MAX || !handler) {
		return -1;
	}
	if (syscall_handler_table[index]) {
		return -1;
	}
	syscall_handler_table[index] = handler;
}

int syscall_invoke(int index, int a, int b, int c) {
	if (syscall_handler_table[index]) {
		return (*syscall_handler_table[index])(a, b, c);
	}
	printf("Unhandled System call %d (param=%x, %x, %x)\n", index, a, b, c);
	return -1;
}

