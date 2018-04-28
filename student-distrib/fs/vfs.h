/**
 *	@file fs/vfs.h
 *
 *	Structures related to the system virtual filesystem.
 */
#ifndef FS_VFS_H
#define FS_VFS_H

#include "../types.h"
#include "../../libc/include/sys/stat.h"
#include "pathname.h"


#define VFS_FILENAME_LEN	32	///< Maximum filename length
#define VFS_MAX_OPEN_FILES	256 ///< System-wide open files limit

#define FTYPE_REGULAR	'f'	///< File type: regular file
#define FTYPE_DIRECTORY	'd'	///< File type: directory
#define FTYPE_SYMLINK	'l'	///< File type: symbolic link
#define FTYPE_DEVICE	'p'	///< File type: special device file

// Forward declarations
struct s_file;
struct s_inode;
struct s_super_block;
struct s_file_operations;
struct s_inode_operations;
struct s_super_operations;
struct dirent;

struct s_file_system;
struct s_vfsmount;

/**
 *	Opened file operations
 *
 *	Driver operations on opened files (i.e., file contents), mainly reading and
 *	writing different types of files.
 */
typedef struct s_file_operations {
	/**
	 *	Initialize a file for open
	 *
	 *	The caller will fill in default values for all fields except
	 *	`private_data` prior to calling this function. This function should not
	 *	modify them if unnecessary (especially `open_count`). This function
	 *	should initialize the `private_data` field if it needs to. This function
	 *	can also set a special f_op for the file if necessary.
	 *
	 *	@param inode: the i-node the file is opened from
	 *	@param file: the newly-opened file
	 *	@return 0 on success, or the negative of an errno on failure
	 */
	int (*open)(struct s_inode *inode, struct s_file *file);

	/**
	 *	Clean-up an opened file for close
	 *
	 *	Similar to `open`, this function should only release its `private_data`
	 *	if set during `open`. The caller will release and rely on the other
	 *	fields for clean-up.
	 *
	 *	@param inode: the i-node the file is opened from
	 *	@param file: the file to be closed
	 *	@return 0 on success, or the negative of an errno on failure
	 */
	int (*release)(struct s_inode *inode, struct s_file *file);

	/**
	 *	Read data into a user buffer
	 *
	 *	@param file: the file to read
	 *	@param buf: the buffer to read into
	 *	@param count: maximum number of bytes, or size of buffer
	 *	@param offset: pointer to file pointer. This function must use this
	 *				   parameter, not the `pos` field in `file`. Although it
	 *				   will point to `pos` most of the time, VFS may specify
	 *				   a different value and must preserve `pos`, such as in
	 *				   `pread`
	 *	@return number of bytes read, or the negative of an errno on failure
	 */
	ssize_t (*read)(struct s_file *file, uint8_t *buf, size_t count,
					off_t *offset);

	/**
	 *	Write data from user buffer into file
	 *
	 *	@param file: the file to write
	 *	@param buf: the data to write
	 *	@param count: the size of buffer
	 *	@param offset: see `offset` of `read`
	 *	@return number of bytes written, or the negative of an errno on failure
	 */
	ssize_t (*write)(struct s_file *file, uint8_t *buf, size_t count,
					off_t *offset);

	/**
	 *	Seek to a given position
	 *
	 *	@param file: the file to seek
	 *	@param offset: the new file pointer, relative to `whence`
	 *	@param whence: reference frame for `offset`. See `SEEK_*` macros.
	 *	@return the absolute offset on success, or the negative of an errno on
	 *	failure.
	 */
	off_t (*llseek)(struct s_file *file, off_t offset, int whence);

	/**
	 *	Read the directory file for dir entires.
	 *
	 *	`dirent` will be used to return data as well as track progress. For the
	 *	first iteration, the `index` field of `dirent` must contain -1. For
	 *	subsequent reads, the `index` field will contain the index of the last
	 *	visited index. Index does not have to be consecutive, but the same index
	 *	must read out the same dir entry. Driver may also utilize the `data`
	 *	field to store private data, but it will not be notified of the deletion
	 *	of `dirent`.
	 *
	 *	@param file: the file to iterate, must be a directory
	 *	@param dirent: the dirent, whose `index` field is used to track
	 *				   iteration progress. The output dir entry will be written
	 *				   to the `ino` and `filename` field.
	 *	@return 0 on success, or the negative of an errno on failure.
	 *			Specifically, -ENOENT when iteration has finished
	 */
	int (*readdir)(struct s_file *file, struct dirent *dirent);
} file_operations_t;

/**
 *	Inode metadata operations
 *
 *	Driver operations on inodes (not yet opened files, metadata only). Mainly
 *	used by directory peeks and metadata operations.
 */
typedef struct s_inode_operations {
	/**
	 *	Find file with name in directory
	 *
	 *	@param inode: current inode, must be a directory
	 *	@param filename: the name to lookup
	 *	@return the i-number of the file, or the negative of an errno on failure
	 */
	ino_t (*lookup)(struct s_inode *inode, const char *filename);

	/**
	 *	Read symbolic link of inode
	 *
	 *	@param inode: current inode, must be a symbolic link
	 *	@param buf: the buffer to read the linked path into, must be big enough
	 *				to hold a path (PATH_MAX_LEN+1)
	 */
	int (*readlink)(struct s_inode *inode, char buf[PATH_MAX_LEN + 1]);

	// int (*create) (struct inode *,struct dentry *,int, struct nameidata *);
	// int (*link) (struct dentry *,struct inode *,struct dentry *);
	// int (*unlink) (struct inode *,struct dentry *);
	// int (*symlink) (struct inode *,struct dentry *,const char *link);
	// int (*mkdir) (struct inode *,struct dentry *,int);
	// int (*rmdir) (struct inode *,struct dentry *);
	// int (*mknod) (struct inode *,struct dentry *,int,dev_t);
	// int (*rename) (struct inode *, struct dentry *, struct inode *, struct dentry *);
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
	/**
	 *	Create a new i-node with new i-number in the file system.
	 *
	 *	Similar to the principle of `creat`. The driver owns the memory of the
	 *	i-node, but the user must free it with `free_inode` after using it.
	 *
	 *	@param sb: the super block of the file system
	 *	@return pointer to a new index node
	 */
	struct s_inode *(*alloc_inode)(struct s_super_block *sb);

	/**
	 *	Retrieve an i-node from the file system with the given i-number.
	 *
	 *	Similar to the principle of `open`. The driver owns the memory of the
	 *	i-node, but the user must free it with `free_inode` after using it.
	 *
	 *	@param sb: the super block of the file system
	 *	@param ino: the i-number of the i-node to be opened
	 *	@return pointer to a populated i-node with the specified `ino`
	 */
   	struct s_inode *(*open_inode)(struct s_super_block *sb, ino_t ino);

   	/**
   	 *	Release an i-node
   	 *
   	 *	Similar to the principle of `close`. The driver should handle any
   	 *	clean-up when the user finishes using the i-node. The i-node passed in
   	 *	must come from either `alloc_inode` or `open_inode`.
   	 *
	 *	@param inode: the inode pointer from `alloc_inode` or `open_inode`
	 *	@return 0 on success, or the negative of an errno on failure
   	 */
	int (*free_inode)(struct s_inode *inode);

	/**
	 *	Populate/update an i-node
	 *
	 *	The driver reads the `ino` field of the i-node passed in and populates
	 *	the rest of the fields.
	 *
	 *	@param inode: the i-node with field `ino` specified. The rest of the
	 *				  fields will be modified by this call
	 *	@return 0 on success, or the negative of an errno on failure
	 */
	int (*read_inode)(struct s_inode *inode);

	/**
	 *	Flush an i-node to permanent storage
	 *
	 *	The driver write the metadata fields in the i-node to the permanent
	 *	index node on its permanent storage specified by the `ino` field. The
	 *	driver may or may not choose to flush certain fields according to its
	 *	file system features or specification.
	 *
	 *	@param inode: the i-node to write
	 *	@return 0 on success, or the negative of an errno on failure
	 */
	int (*write_inode)(struct s_inode *inode);

	/**
	 *	Removes an i-node from permanent storage
	 *
	 *	The driver removes the index node on its permanent storage specified by
	 *	the `ino` field. For instance, when all hardlinks to a file has been
	 *	removed, `drop_inode` would be called to permanently delete the file and
	 *	free up the corresponding disk space.
	 *
	 *	The driver also performs clean-up on the i-node structure. No further
	 *	call to `free_inode` is necessary or allowed.
	 *
	 *	@param inode: the i-node to delete
	 *	@return 0 on success, or the negative of an errno on failure
	 */
	int (*drop_inode)(struct s_inode *inode);
} super_operations_t;

/**
 *	Super block of a mounted file system
 *
 *	Used to access fs-wide operations and entry point into the root of the file
 *	system.
 */
typedef struct s_super_block {
	struct s_file_system *fstype; ///< Reference to file system driver
	super_operations_t *s_op; ///< Super block operations driver
	ino_t root; ///< Root directory i-node
	int open_count; ///< Number of open i-nodes
	int private_data; ///< Private data for drivers
} super_block_t;

/**
 *	A file in the VFS (not opened)
 *
 *	Inode contains metadata information of a file, such as attributes and its
 *	physical location on disk (but NOT filename, which only exists in dirent).
 *	`file_t`s can be opened from inodes.
 *
 *	An inode could be interpreted as a `file_t`, at which the VFS is the user
 *	program and the device driver is the kernel VFS. The device driver is
 *	responsible for the allocation and management of inodes, but the VFS should
 *	tell the device driver to free the inode when it finishes using the inode.
 */
typedef struct s_inode {
	ino_t ino; ///< I-number
	int file_type; ///< File type. See FTPYE_ macros
	off_t size; ///< Size of file in bytes
	int open_count; ///< Number of open files
	int link_count; ///< Number of linked dentrys

	super_block_t *sb; ///< Reference to super block
	file_operations_t *f_op; ///< Default file operations driver
	inode_operations_t *i_op; ///< I-node operations driver

	int perm; ///< File permissions
	uid_t uid; ///< Owner user ID
	gid_t gid; ///< Owner group ID
	time_t atime; ///< Last access date
	time_t mtime; ///< Last modification date
	time_t ctime; ///< Last creation date

	int private_data; ///< Private data for drivers
} inode_t;

/**
 *	An open file
 *
 *	This struct is allocated for each inode opened in a specific mode. It tracks
 *	information for IO on the opened file.
 */
typedef struct s_file {
	inode_t *inode; ///< I-node the file is opened from
	int open_count; ///< Number of open file descriptors
	int mode; ///< The mode the file is opened with
	size_t pos; ///< The file pointer
	file_operations_t *f_op; ///< The file operations driver
	int private_data; ///< Private data for drivers
} file_t;

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
 *	ECE391 wrapper for `syscall_close`. @see syscall_close
 *
 *	This wrapper will return -1 on failure instead of error information.
 *
 *	@param fd: the file descriptor to close
 *	@return 0 on success, or -1 on failure.
 */
int syscall_ece391_close(int fd, int, int);

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
 *	@see syscall_read
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

/**
 *	ECE391 wrapper for `syscall_write`. @see syscall_write
 *
 *	This wrapper will return -1 on failure instead of error information.
 *
 *	@param fd: the file descriptor to write to
 *	@param bufaddr: pointer to the user buffer to write from
 *	@param size: the size of the buffer
 *	@return the number of bytes written, or -1 on failure.
 */
int syscall_ece391_write(int fd, int bufaddr, int count);

/**
 *	System call handler for `lseek`: seek file pointer to given position
 *
 *	@param fd: the file descriptor to write to
 *	@param offset: the new file pointer, relative to `whence`
 *	@param whence: reference frame for `offset`. See `SEEK_*` macros.
 *	@return the absolute offset on success, or the negative of an errno on
 *	failure.
 */
int syscall_lseek(int fd, int offset, int whence);

/**
 *	System call handler for `getdents`: get next entry in directory
 *
 *	@param fd: the file descriptor of the directory to be listed
 *	@param bufaddr: pointer to the `dirent` structure. Consecutive calls should
 *					pass the same `dirent`, as it is used to track the state of
 *					the iteration.
 *	@return: 0 on success, or negative of an errno on failure
 */
int syscall_getdents(int fd, int bufaddr, int);

/**
 *	System call handler for `stat`: get file status
 *
 *	@param path: char string of the path
 *	@param stat_in: pointer to a user allocated stat structure
 *	@return 0 for success, negative value for errors
 */
int syscall_stat(int path, int stat_in, int);

/**
 *	System call handler for `fstat`: get file status
 *
 *	@param fd: fd of the file, of course, a valid one
 *	@param stat_in: pointer to a user allocated stat structure
 *	@return 0 for success, negative value for errors
 */
int syscall_fstat(int fd, int stat_in, int);

/**
 *	System call handler for `lstat`: get file status
 *
 *	@param path: char string of the path
 *	@param stat: pointer to a user allocated stat structure
 *	@return 0 for success, negative value for errors
 *
 *	@note not implemented yet
 */
int syscall_lstat(int path, int stat, int);

/**
 *	System call handler for `chmod`: Change permission of file
 *
 *	@param fd: the file descriptor
 *	@param mode: the new mode
 *	@return 0 on success, or the negative of an errno on failure.
 */
int syscall_chmod(int fd, int mode, int);

/**
 *	System call handler for `chown`: Change owner of file
 *
 *	@param fd: the file descriptor
 *	@param uid: the new owner user ID
 *	@param gid: the new owner group ID
 *	@return 0 on success, or the negative of an errno on failure.
 */
int syscall_chown(int fd, int uid, int gid);

/**
 *	System call handler for `link`: Create new hard link
 *
 *	@param path1p: address of path to the file to create link to (source)
 *	@param path2p: address of path the link (destination)
 *	@return 0 on success, or the negative of an errno on failure.
 */
int syscall_link(int path1p, int path2p, int);

/**
 *	System call handler for `unlink`: Delete file (remove directory entry)
 *
 *	@param pathp: address of the path to the file to unlink
 *	@return 0 on success, or the negative of an errno on failure.
 */
int syscall_unlink(int pathp, int, int);

/**
 *	System call handler for `symlink`: Create symbolic link to file
 *
 *	@param path1p: address of path to the file to create link to (source)
 *	@param path2p: address of path the link (destination)
 *	@return 0 on success, or the negative of an errno on failure.
 */
int syscall_symlink(int path1p, int path2p, int);

/**
 *	System call handler for `readlink`: Read the contents of symbolic link
 *
 *	@param pathp: address of path to the symlink file
 *	@param bufp: address of the buffer to read into
 *	@param count: the maximum number of bytes to read
 *	@return the number of bytes read, or the negative of an errno on failure.
 */
int syscall_readlink(int pathp, int bufp, int bufsize);

/**
 *	System call handler for `truncate`: Truncate or extend a file to a specified
 *	length
 *
 *	@param fd: the file descriptor
 *	@param length: the new length of the file
 *	@return 0 on success, or the negative of an errno on failure.
 */
int syscall_truncate(int fd, int length, int);

/**
 *	Move file
 *
 *	@param oldpathp: address of path to file to move
 *	@param newpathp: address of new name/location of file
 *	@return 0 on success, or the negative of an errno on failure
 */
int syscall_rename(int oldpathp, int newpathp, int);

/**
 *	Get current working directory
 *
 *	@param bufp: the buffer to read cwd into
 *	@param size: the max size of the buffer
 *	@return 0 on success, or the negative of an errno on failure
 */
int syscall_getcwd(int bufp, int size, int);

/**
 *	System call handler for `chdir`: Change current working directory
 *
 *	@param pathp: address of path to the new working directory
 *	@return 0 on success, or the negative of an errno on failure.
 */
int syscall_chdir(int fd, int, int);

/**
 *	System call handler for `mkdir`: Create new directory
 *
 *	@param pathp: address of path to the new directory
 *	@param mode: permission bits of the new directory file
 *	@return 0 on success, or the negative of an errno on failure.
 */
int syscall_mkdir(int pathp, int mode, int);

/**
 *	Remove directory
 *
 *	@param pathp: path to the directory to be removed
 *	@return 0 on success, or -1 on failure. Set errno
 */
int syscall_rmdir(int pathp, int, int);

// For consistent include order
#include "fstab.h"

#endif
