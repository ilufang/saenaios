/**
 *	@file unistd.h
 *
 *	File descriptor IO
 */
#ifndef UNISTD_H
#define UNISTD_H

#include "sys/types.h"

#define SEEK_SET	0 ///< Seek relative to the start of the file
#define SEEK_CUR	1 ///< Seek relative to the current position
#define SEEK_END	2 ///< Seek relative to the end of file

/**
 *	Read bytes from an open file
 *
 *	@param fd: the file descriptor to read from
 *	@param buf: pointer to the buffer to read into
 *	@param count: the maximum number of bytes to read
 *	@return the number of bytes read, or the negative of an errno on failure.
 */
ssize_t read(int fd, void *buf, size_t count);

/**
 *	Write bytes to an open file
 *
 *	This write call is always synchronous, i.e., it will not return until all
 *	data has been acknowledged by the driver.
 *
 *	@param fd: the file descriptor to write to
 *	@param buf: pointer to the data buffer
 *	@param count: the size of the buffer
 *	@return the number of bytes written, or the negative of an errno on failure.
 */
ssize_t write(int fd, const void *buf, size_t count);


/**
 *	System call handler for `lseek`: seek file pointer to given position
 *
 *	@param fd: the file descriptor to write to
 *	@param offset: the new file pointer, relative to `whence`
 *	@param whence: reference frame for `offset`. See `SEEK_*` macros.
 *	@return the absolute offset on success, or the negative of an errno on
 *	failure.
 */
off_t lseek(int fd, off_t offset, int whence);

/**
 *	Duplicate current process
 *
 *	After calling fork, the current process will be duplicated by creating new
 *	page table entries referencing the same physical addresses. The first
 *	attempt to write to these pages will then trigger copy-on-write.
 *
 *	@return the new PID to the calling process on successful, 0 to the newly
 *			forked process, or the negative of an errno on failure
 */
pid_t fork();


/**
 *	Reload current process with the given executable image.
 *
 *	@param path: path to the executable file (ELF)
 *	@param argv: list of command-line arguments, the last element must be NULL
 *				 to signify end-of-list
 *	@param envp: list of environmental variables, the last element must be NULL
 *				 to signify end-of-list
 *	@return the negative of an errno on failure. On success, the new process
 *			is launched and the old process will not be present to receive any
 *			return values.
 */
int execve(const char *path, char *const argv[], char *const envp[]);

/**
 *	Terminate the calling process with given exit status code
 *
 *	This function will always succeed. After calling, the calling process will
 *	no longer exist to receive any returned values.
 *
 *	@param status: the exit status code
 */
void _exit(int status);

#endif
