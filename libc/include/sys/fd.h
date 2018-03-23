/**
 *	@file sys/fd.h
 *
 *	System calls to open and operate on file descriptors.
 */
#ifndef SYS_FD_H
#define SYS_FD_H

#define O_RDONLY	0x1
#define O_WRONLY	0x2
#define O_RDWR		0x3

int open(const char *pathname, int flags, int mode);
int close(int fd);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);

#endif
