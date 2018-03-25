/**
 *	@file syscall.h
 *
 *	System call handler registration and dispatcher
 */
#ifndef BOOT_SYSCALL_H
#define BOOT_SYSCALL_H

/**
 *	System call handler
 */
typedef int (*syscall_handler)(int, int, int);

/**
 *	Register a system call handler
 *
 *	@note To avoid confusion and syscall number collisions, this function should
 *		  not be called outside `syscall_register_all`.
 *	@see syscall_register_all
 *
 *	@param index: The system call number
 *	@param handler: The handler function
 *	@return 0 on success, -1 if parameter is invalid or the syscall number
 *			is already in use
 */
int syscall_register(int index, syscall_handler handler);

/**
 *	Initialize all system calls
 *
 *	@note This function is designed to hold all calls to `syscall_register`.
 *		  This function will be called by the OS entry function once, and no one
 *		  else should invoke this function. Rather, anyone who wish to register
 *		  a system call handler should modify the implementation of this
 *		  function in `syscall.c` and define its system call number in libc's
 *		  `src/syscall.h` to prevent number conflicts. The client-side caller
 *		  implementation should be placed in libc.
 *	@see syscall_register
 */
void syscall_register_all();

/**
 *	Dispatch a system call to the given system call number
 *
 *	@note This function is used by the INT 0x80 handler only. If you wish to
 *		  invoke a system call, use interfaces in libc.
 *
 *	@param index: the system call number
 *	@param a, b, c: the three 32-bit parameters
 */
int syscall_invoke(int index, int a, int b, int c);

#endif
