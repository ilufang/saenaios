/**
 *	@file fs/vfs.h
 *
 *	Structures related to the system virtual filesystem.
 */
#ifndef FS_VFS_H
#define FS_VFS_H

#include "../types.h"

#include "pathname.h"


#define VFS_FILENAME_LEN	32
#define VFS_MAX_OPEN_FILES	256

#define FTYPE_REGULAR	1
#define FTYPE_DIRECTORY	2
#define FTYPE_SYMLINK	3
#define FTYPE_DEVICE	4

struct s_file;
struct s_inode;
struct s_super_block;
struct s_dentry;
struct s_dirent;
struct s_file_operations;
struct s_inode_operations;
struct s_super_operations;

struct s_file_system;
struct s_vfsmount;

typedef struct s_file_operations {
	int (*open)(struct s_inode *inode, struct s_file *file);
	int (*release)(struct s_inode *inode, struct s_file *file);
	ssize_t (*read)(struct s_file *, uint8_t *buf, size_t count, size_t *offset);
	ssize_t (*write)(struct s_file *, uint8_t *buf, size_t count, size_t *offset);
	void (*readdir)(struct s_file *, struct s_dirent *dirent);
} file_operations_t;

typedef struct s_inode_operations {
	struct dentry * (*lookup) (struct s_inode *inode, const char *filename);
	// int (*permission) (struct inode *, int, unsigned int);
	int (*readlink) (struct s_dentry *dentry, char *buf, int size);
	// int (*create) (struct inode *,struct dentry *,int, struct nameidata *);
	// int (*link) (struct dentry *,struct inode *,struct dentry *);
	// int (*unlink) (struct inode *,struct dentry *);
	// int (*symlink) (struct inode *,struct dentry *,const char *link);
	// int (*mkdir) (struct inode *,struct dentry *,int);
	// int (*rmdir) (struct inode *,struct dentry *);
	// int (*mknod) (struct inode *,struct dentry *,int,dev_t);
	// int (*rename) (struct inode *, struct dentry *,
			// struct inode *, struct dentry *);
	// void (*truncate) (struct inode *);
	// int (*setattr) (struct dentry *, struct iattr *);
	// int (*getattr) (struct vfsmount *mnt, struct dentry *, struct kstat *);
} inode_operations_t;

typedef struct s_super_operations {
   	struct s_inode *(*alloc_inode)(struct s_super_block *sb);
	void (*destroy_inode)(struct s_inode *);
	int (*write_inode) (struct s_inode *);
	int (*drop_inode) (struct s_inode *);
} super_operations_t;

typedef struct s_super_block {
	struct s_file_system *fstype;
	super_operations_t *s_op;
	struct s_dentry *root;
	int open_count;
} super_block_t;

/**
 *	A file in the VFS, not opened. Might be written to disk.
 */
typedef struct s_inode {
	uint32_t ino;
	int file_type;
	int open_count;
	uint16_t mode;
	super_block_t *sb;
	file_operations_t *f_op;
	inode_operations_t *i_op;
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

typedef struct s_dentry {
	char filename[VFS_FILENAME_LEN];
	inode_t *inode;
} dentry_t;

typedef struct s_dirent {
	dentry_t *dentry;
	union {
		int index;
		void *data;
	} dirent;
} dirent_t;

file_t *vfs_open_file(inode_t *inode, int mode);

int vfs_close_file(file_t *file);

// System call handlers
int syscall_open(int pathaddr, int flags, int mode);
int syscall_ece391_open(int pathaddr, int, int);
int syscall_close(int fd, int, int);
int syscall_ece391_read(int fd, int bufaddr, int size);
int syscall_read(int fd, int bufaddr, int size);
int syscall_write(int fd, int bufaddr, int size);

#include "fstab.h"

#endif
