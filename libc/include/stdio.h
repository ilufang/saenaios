/**
 *	@file stdio.h
 *
 *	Standard IO
 */
#ifndef STDIO_H
#define STDIO_H

/**
 *	Rename a file (directory entry)
 *
 *	@param old: the file to rename/move
 *	@param new: the new name/location of the file
 *	@return 0 on success, or -1 on failure. Set errno
 */
int rename(const char *old, const char *new);

/**
 *	Get working directory pathname
 *
 *	@param buf: pointer to the buffer to read into
 *	@param size: the maximum number of bytes to read
 *	@return: buf on success, NULL on failure. Set errno
 */
char *getcwd(char *buf, size_t size);

#endif
