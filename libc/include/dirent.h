/**
 *	@file dirent.h
 *
 *	Directory operations
 */
#ifndef DIRENT_H
#define DIRENT_H

/**
 *	Directory entity used to iterate a directory
 *
 *	This struct is designed similar to the userspace directory listing struct.
 *	It contains both the dentry to be read by the user, and the private data
 *	used by `readdir` to track its current position in the iteration.
 */
struct dirent {
	long ino; ///< I-number of file
	char filename[32 + 1]; ///< Name of file (see VFS_FILENAME_LEN)
	int index; ///< Index number in the directory
	int data; ///< Custom private data
};

/**
 *	Get next entry in directory
 *
 *	This is NOT POSIX-compliant, and users should use the standard directory
 *	functions instead. @see
 *
 *	@param fd: the file descriptor of the directory to be listed
 *	@param buf: pointer to the `dirent` structure. Consecutive calls should
 *				pass the same `dirent`, as it is used to track the state of the
 *				iteration.
 *	@return: 0 on success, or negative of an errno on failure
 */
int getdents(int fd, struct dirent *buf);

/**
 *	DIR structure for POSIX-compliant directory calls
 */
typedef struct s_dir {
	int count; ///< Open count
	int fd; ///< Open file descriptor
	struct dirent dent; ///< `dirent` structure
} DIR;

/**
 *	Open a directory for listing
 *
 *	@param filename: path to the directory to open
 *	@return pointer to DIR structure, or NULL on error
 */
DIR *opendir(const char *filename);

/**
 *	Open a directory from an opened fd for listing
 *
 *	@param fd: the file descriptor to the directory to open
 *	@return pointer to DIR structure, or NULL on error
 */
DIR *fdopendir(int fd);

/**
 *	Read an entry in the opened directory
 *
 *	@param dirp: pointer to the opened DIR structure
 *	@return pointer to the populated `dirent`. User should only read it.
 */
struct dirent *readdir(DIR *dirp);

/**
 *	Find the current location in the directory iteration
 *
 *	@param dirp: pointer to the opened DIR structure
 *	@return an index, to be used by seekdir. The index may or may not be
 *			contiguous.
 *	@see seekdir
 */
long telldir(DIR *dirp);

/**
 *	Seek to a previously `tell`ed location in the directory iteration
 *
 *	@param dirp: pointer to the opened DIR structure
 *	@param loc: the index previously returned by `telldir`. Manual manipulation
 *				of the location causes undefined behaviors
 *	@see telldir
 */
void seekdir(DIR *dirp, long loc);

/**
 *	Seek to the beginning location in the directory iteration
 *
 *	@param dirp: pointer to the opened DIR structure
 */
void rewinddir(DIR *dirp);

/**
 *	Finish listing and release a DIR struct allocated by `opendir`
 *
 *	@param dirp: pointer to the opened DIR structure
 *	@return 0 on success, or the negative of an errno on failure
 */
int closedir(DIR *dirp);

/**
 *	Find the fd of an open directory
 *
 *	@param dirp: pointer to the opened DIR structure
 *	@return the file descriptor underlying the open directory
 */
int dirfd(DIR *dirp);

#endif
