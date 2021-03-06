#include "syscall.h"

#define SYSCALL_NUMBER_MAX 256

#include "../lib.h"
#include "../fs/vfs.h"
#include "../proc/task.h"
#include "../proc/signal.h"

#include "../../libc/src/syscalls.h" // Definitions from libc
#include "../terminal_driver/terminal_out_driver.h"

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
	if (index < 0 || index >= SYSCALL_NUMBER_MAX) {
		printf("Bad syscall number %d\n", index);
		return -1;
	}
	if (syscall_handler_table[index]) {
		return (*syscall_handler_table[index])(a, b, c);
	}
	printf("Unhandled System call %d (param=%x, %x, %x)\n", index, a, b, c);
	return -1;
}

void syscall_register_all() {
	// ECE 391 System calls

	// ece391_halt
	syscall_register(1, syscall_ece391_halt);
	// ece391_execute
	syscall_register(2, syscall_ece391_execute);
	// ece391_read
	syscall_register(3, syscall_ece391_read);
	// ece391_write
	syscall_register(4, syscall_ece391_write);
	// ece391_open
	syscall_register(5, syscall_ece391_open);
	// ece391_close
	syscall_register(6, syscall_ece391_close);
	// ece391_getargs
	syscall_register(7, syscall_ece391_getargs);
	// ece391_vidmap
	syscall_register(8, syscall_ece391_vidmap);
	// ece391_set_handler
	syscall_register(9, syscall_ece391_set_handler);
	// ece391_sigreturn
	syscall_register(10, syscall_ece391_sigreturn);

	// Internal calls
	syscall_register(15, task_make_initd);

	// POSIX System calls

	// VFS
	syscall_register(SYSCALL_OPEN, syscall_open);
	syscall_register(SYSCALL_CLOSE, syscall_close);
	syscall_register(SYSCALL_READ, syscall_read);
	syscall_register(SYSCALL_WRITE, syscall_write);
	syscall_register(SYSCALL_MOUNT, syscall_mount);
	syscall_register(SYSCALL_UMOUNT, syscall_umount);
	syscall_register(SYSCALL_GETDENTS, syscall_getdents);
	syscall_register(SYSCALL_LSEEK, syscall_lseek);
	syscall_register(SYSCALL_STAT, syscall_stat);
	syscall_register(SYSCALL_FSTAT, syscall_fstat);
	syscall_register(SYSCALL_LSTAT, syscall_lstat);

	// Process
	syscall_register(SYSCALL_FORK, syscall_fork);
	syscall_register(SYSCALL__EXIT, syscall__exit);
	syscall_register(SYSCALL_EXECVE, syscall_execve);
	syscall_register(SYSCALL_WAITPID, syscall_waitpid);
	syscall_register(SYSCALL_GETPID, syscall_getpid);
	syscall_register(SYSCALL_BRK, syscall_brk);
	syscall_register(SYSCALL_SBRK, syscall_sbrk);

	// Signals
	syscall_register(SYSCALL_KILL, syscall_kill);
	syscall_register(SYSCALL_SIGACTION, syscall_sigaction);
	syscall_register(SYSCALL_SIGSUSPEND, syscall_sigsuspend);
	syscall_register(SYSCALL_SIGPROCMASK, syscall_sigprocmask);
	
	// VFS Security
	syscall_register(SYSCALL_CHMOD, syscall_chmod);
	syscall_register(SYSCALL_CHOWN, syscall_chown);
	syscall_register(SYSCALL_LINK, syscall_link);
	syscall_register(SYSCALL_UNLINK, syscall_unlink);
	syscall_register(SYSCALL_SYMLINK, syscall_symlink);
	syscall_register(SYSCALL_READLINK, syscall_readlink);
	syscall_register(SYSCALL_TRUNCATE, syscall_truncate);
	syscall_register(SYSCALL_RENAME, syscall_rename);
	syscall_register(SYSCALL_GETCWD, syscall_getcwd);
	syscall_register(SYSCALL_CHDIR, syscall_chdir);
	syscall_register(SYSCALL_MKDIR, syscall_mkdir);
	syscall_register(SYSCALL_RMDIR, syscall_rmdir);
	syscall_register(SYSCALL_IOCTL, syscall_ioctl);
	
	// Process security
	syscall_register(SYSCALL_SETUID, syscall_setuid);
	syscall_register(SYSCALL_SETGID, syscall_setgid);
	syscall_register(SYSCALL_GETUID, syscall_getuid);
	syscall_register(SYSCALL_GETGID, syscall_getgid);
	
}
