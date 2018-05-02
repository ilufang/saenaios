/**
 *	@file fcntl.h
 *
 *	Opening and closing file descriptors
 */
#ifndef FCNTL_H
#define FCNTL_H

#define O_RDONLY	0x0000
#define O_WRONLY	0x0001
#define O_RDWR		0x0002
#define O_APPEND	0x0008
#define O_CREAT		0x0200
#define O_TRUNC		0x0400
#define O_EXCL		0x0800
#define O_SYNC		0x2000

/**
 *	Opens a file from a given path
 *
 *	@param pathname: path to the file to be opened
 *	@param flags: open flags (mode and options)
 *	@param mode: file mode for `creat`
 *	@return The file descriptor on success, or the negative value of errno on
 *			failure
 */
int open(const char *pathname, int flags, int mode);

/**
 *	Closes an open fd.
 *
 *	@param fd: the file descriptor to close
 *	@return 0 on success, or the negative of an errno on failure.
 */
int close(int fd);

#endif
