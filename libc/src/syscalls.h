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

#define SYSCALL_OPEN		16
#define SYSCALL_CLOSE		17
#define SYSCALL_READ		18
#define SYSCALL_WRITE		19
#define SYSCALL_MOUNT		20
#define SYSCALL_UMOUNT		21
#define SYSCALL_GETDENTS	22

struct __attribute__((__packed__)) sys_mount_opts {
	const char *source;
	unsigned long mountflags;
	const char *opts;
};

#endif
