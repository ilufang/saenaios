#include "syscall.h"

#define SYSCALL_NUMBER_MAX 256

#include "../lib.h"
#include "../fs/vfs.h"

#include "../../libc/src/syscalls.h" // Definitions from libc

syscall_handler syscall_handler_table[SYSCALL_NUMBER_MAX];

int syscall_register(int index, syscall_handler handler) {
	if (index < 0 || index > SYSCALL_NUMBER_MAX || !handler) {
		return -1;
	}
	if (syscall_handler_table[index]) {
		return -1;
	}
	syscall_handler_table[index] = handler;
	return 0;
}

int syscall_invoke(int index, int a, int b, int c) {
	if (syscall_handler_table[index]) {
		return (*syscall_handler_table[index])(a, b, c);
	}
	printf("Unhandled System call %d (param=%x, %x, %x)\n", index, a, b, c);
	return -1;
}

void syscall_register_all() {
	// ECE 391 System calls

	// ece391_halt
	syscall_register(1, NULL);
	// ece391_execute
	syscall_register(2, NULL);
	// ece391_read
	syscall_register(3, syscall_ece391_read);
	// ece391_write
	syscall_register(4, syscall_write);
	// ece391_open
	syscall_register(5, syscall_ece391_open);
	// ece391_close
	syscall_register(6, syscall_close);
	// ece391_getargs
	syscall_register(7, NULL);
	// ece391_vidmap
	syscall_register(8, NULL);
	// ece391_set_handler
	syscall_register(9, NULL);
	// ece391_sigreturn
	syscall_register(10, NULL);

	// POSIX System calls
	
	// VFS
	syscall_register(SYSCALL_OPEN, syscall_open);
	syscall_register(SYSCALL_CLOSE, syscall_close);
	syscall_register(SYSCALL_READ, syscall_read);
	syscall_register(SYSCALL_WRITE, syscall_write);
	syscall_register(SYSCALL_MOUNT, syscall_mount);
	syscall_register(SYSCALL_UMOUNT, syscall_umount);
}
