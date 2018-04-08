/**
 *	@file unistd.h
 *
 *	File descriptor IO
 */
#ifndef UNISTD_H
#define UNISTD_H

#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2

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
 *	Seek to a given position in an open file
 *
 *	@param fd: the file descriptor to seek
 *	@param offset: the offset relative to `whence`
 *	@param whence: the reference frame for `offset`. See SEEK_ macros
 */
off_t lseek(int fildes, off_t offset, int whence);

/**
 *	Change permission of file
 *
 *	@param path: path to the file
 *	@param mode: the new mode to set
 *	@return 0 on success, or -1 on failure. Set errno
 */
int chmod(const char *path, mode_t mode);

/**
 *	Change permission of file from file descriptor
 *
 *	@param fd: the file descriptor
 *	@param mode: the new mode
 *	@return 0 on success, or -1 on failure. Set errno
 */
int fchmod(int fd, mode_t mode);

/**
 *	Change owner of file
 *
 *	@param path: path to the file
 *	@param uid: the new owner user ID
 *	@param gid: the new owner group ID
 *	@return 0 on success, or -1 on failure. Set errno
 */
int chown(const char *path, uid_t uid, gid_t gid);

/**
 *	Change owner of file from file descriptor
 *
 *	@param fd: the file descriptor
 *	@param uid: the new owner user ID
 *	@param gid: the new owner group ID
 *	@return 0 on success, or -1 on failure. Set errno
 */
int fchown(int fd, uid_t uid, gid_t gid);

/**
 *	Create new hard link
 *
 *	@param path1: the file to create link to (source)
 *	@param path2: the path of the link (destination)
 *	@return 0 on success, or -1 on failure. Set errno
 */
int link(const char *path1, const char *path2);

/**
 *	Delete file (remove directory entry)
 *
 *	@param path: the file to unlink
 *	@return 0 on success, or -1 on failure. Set errno
 */
int unlink(const char *path);

/**
 *	Create symbolic link to file
 *
 *	@param path1: the file to create link to (source)
 *	@param path2: the path of the link (destination)
 *	@return 0 on success, or -1 on failure. Set errno
 */
int symlink(const char *path1, const char *path2);

/**
 *	Read the contents of symbolic link
 *	@param path: the symlink file
 *	@param buf: pointer to the buffer to read into
 *	@param count: the maximum number of bytes to read
 *	@return the number of bytes read, or -1 on failure. Set errno.
 */
ssize_t readlink(const char *restrict path, char *restrict buf, size_t bufsize);

/**
 *	Truncate or extend a file from a file descriptor to a specified length
 *
 *	@param fd: the file descriptor
 *	@param length: the new length of the file
 *	@return 0 on success, or -1 on failure. Set errno
 */
int ftruncate(int fd, off_t length);

/**
 *	Truncate or extend a file to a specified length
 *
 *	@param path: the file
 *	@param length: the new length of the file
 *	@return 0 on success, or -1 on failure. Set errno
 */
int truncate(const char *path, off_t length);

/**
 *	Change current working directory
 *
 *	@param path: path to the new working directory
 *	@return 0 on success, or -1 on failure. Set errno
 */
int chdir(const char *path);

/**
 *	Change current working directory from descriptor
 *
 *	@param fd: file descriptor of the new working directory
 *	@return 0 on success, or -1 on failure. Set errno
 */
int fchdir(int fd);

/**
 *	Create new directory
 *
 *	@param path: path to the new directory
 *	@param mode: permission bits of the new directory file
 *	@return 0 on success, or -1 on failure. Set errno
 */
int mkdir(const char *path, mode_t mode);

/**
 *	Remove directory
 *
 *	@param path: path to the directory to be removed
 *	@return 0 on success, or -1 on failure. Set errno
 */
int rmdir(const char *path);

#endif
