#ifndef BOOT_SYSCALL_H
#define BOOT_SYSCALL_H

typedef int (*syscall_handler)(int, int, int);

int syscall_register(int index, syscall_handler handler);
void syscall_register_all();

int syscall_invoke(int index, int a, int b, int c);

#endif
