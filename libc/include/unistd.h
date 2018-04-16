/**
 *	@file unistd.h
 *
 *	File descriptor IO
 */
#ifndef UNISTD_H
#define UNISTD_H

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

#endif
