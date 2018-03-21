/**
 *	@file fs/vfs.h
 *
 *	Structures related to the system virtual filesystem.
 */
#ifndef FS_VFS_H
#define FS_VFS_H

#include "../types.h"

struct s_file;
struct s_inode;
struct s_file_operations;

typedef struct s_file_operations {
	int (*open)(struct s_inode *inode, struct s_file *file);
	int (*release)(struct s_inode *inode, struct s_file *file);
	ssize_t (*read)(struct s_file *, uint8_t *buf, size_t count, size_t *offset);
	ssize_t (*write)(struct s_file *, uint8_t *buf, size_t count, size_t *offset);
} file_operations_t;

/**
 *	A file in the VFS, not opened. Might be written to disk.
 */
typedef struct s_inode {
	uint32_t ino;
	int open_count;
	uint16_t mode;
	file_operations_t *f_op;
} inode_t;

/**
 *	An open file, pointed to by a file descriptor
 */
typedef struct s_file {
	inode_t *inode;
	int open_count;
	int mode; ///< The mode the file is opened with
	size_t pos; ///< The file pointer
	file_operations_t *f_op; ///< The file driver
} file_t;

// System call handlers
int syscall_open(int pathaddr, int flags, int mode);
int syscall_ece391_open(int pathaddr, int, int);
int syscall_close(int fd, int, int);
int syscall_read(int fd, int bufaddr, int size);
int syscall_write(int fd, int bufaddr, int size);

#endif
