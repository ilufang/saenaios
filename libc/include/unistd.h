/**
 *	@file unistd.h
 *
 *	File descriptor IO
 */
#ifndef UNISTD_H
#define UNISTD_H

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

#endif
