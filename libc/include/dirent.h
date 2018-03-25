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
	int count;
	int fd;
	struct dirent dent;
} DIR;

DIR *opendir(const char *filename);

DIR *fdopendir(int fd);

struct dirent *readdir(DIR *dirp);

int readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result);

long telldir(DIR *dirp);

void seekdir(DIR *dirp, long loc);

void rewinddir(DIR *dirp);

int closedir(DIR *dirp);

int dirfd(DIR *dirp);

#endif
