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

#define SYSCALL_LSEEK		25
#define SYSCALL_CHMOD		26
#define SYSCALL_CHOWN		27
#define SYSCALL_LINK		28
#define SYSCALL_UNLINK		29
#define SYSCALL_SYMLINK		30
#define SYSCALL_READLINK	31
#define SYSCALL_TRUNCATE	32
#define SYSALL_STAT			33
#define SYSCALL_RENAME		34
#define SYSCALL_GETCWD		35
#define SYSCALL_CHDIR		36
#define SYSCALL_MKDIR		37
#define SYSCALL_RMDIR		38

struct __attribute__((__packed__)) sys_mount_opts {
	const char *source;
	unsigned long mountflags;
	const char *opts;
};

/**
 *	Execute syscall linkage wrapper (used for testing)
 *
 *	@param command: the command to execute
 */
int mp3_execute(unsigned char* command);


/**
 *	Halt syscall linkage wrapper (used for testing)
 *
 *	@return the return value from the halted process
 */
int mp3_halt(unsigned char status);

int mp3_open(unsigned char* fname);

int mp3_close(int fd);

int mp3_read(int fd, unsigned char* buf, int numbyte);

int mp3_write(int fd, unsigned char* buf, int numbyte);

#endif
