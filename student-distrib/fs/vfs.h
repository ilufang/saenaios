/**
 *	@file fs/vfs.h
 *
 *	Structures related to the system virtual filesystem.
 */
#ifndef FS_VFS_H
#define FS_VFS_H

#include "../types.h"

#include "pathname.h"


#define VFS_FILENAME_LEN	32	///< Maximum filename length
#define VFS_MAX_OPEN_FILES	256 ///< System-wide open files limit

#define FTYPE_REGULAR	1	///< File type: regular file
#define FTYPE_DIRECTORY	2	///< File type: directory
#define FTYPE_SYMLINK	3	///< File type: symbolic link
#define FTYPE_DEVICE	4	///< File type: special device file

// Forward declarations
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

/**
 *	Opened file operations
 *
 *	Driver operations on opened files (i.e., file contents), mainly reading and
 *	writing different types of files.
 */
typedef struct s_file_operations {
	int (*open)(struct s_inode *inode, struct s_file *file);
	int (*release)(struct s_inode *inode, struct s_file *file);
	ssize_t (*read)(struct s_file *, uint8_t *buf, size_t count, size_t *offset);
	ssize_t (*write)(struct s_file *, uint8_t *buf, size_t count, size_t *offset);
	void (*readdir)(struct s_file *, struct s_dirent *dirent);
} file_operations_t;

/**
 *	Inode metadata operations
 *
 *	Driver operations on inodes (not yet opened files, metadata only). Mainly
 *	used by directory peeks and metadata operations.
 */
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

/**
 *	Super block operations
 *
 *	Driver operations on the super block (), mainly the allocation and
 *	management of inode pool for the mounted file system.
 */
typedef struct s_super_operations {
   	struct s_inode *(*alloc_inode)(struct s_super_block *sb);
	void (*destroy_inode)(struct s_inode *);
	int (*write_inode) (struct s_inode *);
	int (*drop_inode) (struct s_inode *);
} super_operations_t;

/**
 *	Super block of a mounted file system
 *
 *	Used to access fs-wide operations and entry point into the root of the file
 *	system.
 */
typedef struct s_super_block {
	struct s_file_system *fstype;
	super_operations_t *s_op;
	struct s_dentry *root;
	int open_count;
} super_block_t;

/**
 *	A file in the VFS (not opened)
 *
 *	Inode contains metadata information of a file, such as attributes and its
 *	physical location on disk (but NOT filename, which only exists in dentry).
 *	`file_t`s can be opened from inodes.
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
 *	An open file
 *
 *	This struct is allocated for each inode opened in a specific mode. It tracks
 *	information for IO on the opened file.
 */
typedef struct s_file {
	inode_t *inode;
	int open_count;
	int mode; ///< The mode the file is opened with
	size_t pos; ///< The file pointer
	file_operations_t *f_op; ///< The file driver
} file_t;

/**
 *	A entry (named file link) of a directory.
 *
 *	This struct represents an element (hard link) in a directory. It tracks
 *	a filename in the directory and the inode of the file.
 */
typedef struct s_dentry {
	char filename[VFS_FILENAME_LEN];
	inode_t *inode;
} dentry_t;

/**
 *	Directory entity used to iterate a directory
 *
 *	This struct is designed similar to the userspace directory listing struct.
 *	It contains both the dentry to be read by the user, and the private data
 *	used by `readdir` to track its current position in the iteration.
 */
typedef struct s_dirent {
	dentry_t *dentry;
	union {
		int index;
		void *data;
	} dirent;
} dirent_t;

/**
 *	System-level `file_t` allocation
 *
 *	Creates an open `file_t` system-wide with a given inode under the given
 *	mode.
 *
 *	@param inode: the inode to open
 *	@param mode: the mode. See `libc`'s sys/fd.h for macro definitions
 *	@return pointer to the `file_t` on success, or NULL on failure (set errno).
 */
file_t *vfs_open_file(inode_t *inode, int mode);

/**
 *	Release an open `file_t`
 *
 *	Decrement open count of the given `file_t`. Frees the `file_t` when all
 *	file descriptors pointing to the `file_t` has been released. Will also
 *	decrement and release the associated inode when all opened files of the
 *	inode is closed.
 *
 *	@param file: pointer to the opened file
 *	@return int: 0 on success. The negative of errno on failure.
 */
int vfs_close_file(file_t *file);

// System call handlers
/**
 *	System call handler for `open`: opens a file from a given path and produces
 *	an open fd number.
 *
 *	@param pathaddr: the path
 *	@param flags: open flags (mode and options)
 *	@param mode: file mode for `creat`
 *	@return The file descriptor on success, or the negative value of errno on
 *			failure
 */
int syscall_open(int pathaddr, int flags, int mode);

/**
 *	System call handler for `ece391_open`: adapter for ECE391 specification of
 *	`open`.
 *
 *	Always open as `IO_RDWR`. If such open fails, retry open as `IO_RDONLY`.
 *	@see syscall_open
 *
 *	@param pathaddr: see `syscall_open`
 *	@return see `syscall_open`
 */
int syscall_ece391_open(int pathaddr, int, int);

/**
 *	System call handler for `close`: closes an open fd.
 *
 *	@param fd: the file descriptor to close
 *	@return 0 on success, or the negative of an errno on failure.
 */
int syscall_close(int fd, int, int);

/**
 *	System call handler for `read`: read bytes from an open fd into a provided
 *	user buffer.
 *
 *	@param fd: the file descriptor to read from
 *	@param bufaddr: pointer to the user buffer to read into
 *	@param size: the maximum number of bytes to read
 *	@return the number of bytes read, or the negative of an errno on failure.
 */
int syscall_read(int fd, int bufaddr, int size);

/**
 *	System call handler for `ece391_read`: adapter for ECE391 specification of
 *	`read`
 *
 *	In addition to regular read, the `ece391_read` can also be performed on an
 *	directory, in which case each call to read will read in the filename of an
 *	entry in the directory.
 *
 *	@param fd, bufaddr, size: see `syscall_read`
 *	@return see `syscall_read`
 */
int syscall_ece391_read(int fd, int bufaddr, int size);

/**
 *	System call handler for `write`: write bytes to an open fd from a provided
 *	user buffer.
 *
 *	This write call is always synchronous, i.e., it will not return until all
 *	data has been acknowledged by the driver.
 *
 *	@param fd: the file descriptor to write to
 *	@param bufaddr: pointer to the user buffer to write from
 *	@param size: the size of the buffer
 *	@return the number of bytes written, or the negative of an errno on failure.
 */
int syscall_write(int fd, int bufaddr, int size);

// For consistent include order
#include "fstab.h"

#endif
