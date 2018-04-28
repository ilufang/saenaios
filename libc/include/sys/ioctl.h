/**
 *	@file sys/ioctl.h
 *
 *	`ioctl` system call
 */
#ifndef SYS_IOCTL_C
#define SYS_IOCTL_C

/**
 *	Control device
 *
 *	@param fd: the file to send commands to
 *	@param cmd: the command
 *	@param arg: the argument
 */
int ioctl(int fd, int cmd, int arg);

#endif
