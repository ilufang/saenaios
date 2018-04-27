/**
 *	@file libc/src/syscalls.h
 *
 *	This file describes shared constants and structures used to pass data for
 *	system calls. This file is included by both userspace programs linking with
 *	libc and kernel code handling the system calls raised from libc.
 */
#ifndef LIBC_SRC_SYSCALLS_H
#define LIBC_SRC_SYSCALLS_H

#include "../include/fcntl.h"
#include "../include/dirent.h"
#define MP_HALT    1
#define MP_EXECUTE 2
#define MP_READ    3
#define MP_WRITE   4
#define MP_OPEN    5
#define MP_CLOSE   6
#define MP_GETARGS 7
#define MP_VIDMAP  8
#define MP_SET_HANDLER  9
#define MP_SIGRETURN  10

#define SYSCALL_OPEN		16
#define SYSCALL_CLOSE		17
#define SYSCALL_READ		18
#define SYSCALL_WRITE		19
#define SYSCALL_MOUNT		20
#define SYSCALL_UMOUNT		21
#define SYSCALL_GETDENTS	22
#define SYSCALL_FORK		23
#define SYSCALL__EXIT		24
#define SYSCALL_EXECVE		25
#define SYSCALL_SIGACTION	26
#define SYSCALL_KILL		27
#define SYSCALL_SIGSUSPEND	28
#define SYSCALL_SIGPROCMASK	29
#define SYSCALL_WAITPID		30
#define SYSCALL_STAT		31
#define SYSCALL_FSTAT		32
#define SYSCALL_LSTAT		33
#define SYSCALL_GETPID		34

struct sys_mount_opts {
	const char *source;
	unsigned long mountflags;
	const char *opts;
} __attribute__((__packed__));

#endif
