/**
 *	@file fcntl.h
 *
 *	Opening and closing file descriptors
 */
#ifndef FCNTL_H
#define FCNTL_H

#define O_RDONLY	0x1		///< Open for reading only
#define O_WRONLY	0x2		///< Open for writing only
#define O_RDWR		0x3		///< Open for reading and writing

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
