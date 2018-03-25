/**
 *	@file fs/fstab.h
 *
 *	The management of specific file systems and mounted instances of them
 */
#ifndef FS_FSTAB_H
#define FS_FSTAB_H

#include "vfs.h"

#define FSTAB_FS_NAME_LEN	16	///< Maximum length of file system type name

/**
 *	The driver for a file system type.
 *
 *	This structure is used by mount when initializing a new mount. The driver
 *	handles the mounting by providing the `get_sb` function that generates a
 *	super block with filled s_op driver functions.
 */
typedef struct s_file_system {
	char name[FSTAB_FS_NAME_LEN + 1]; ///< Name used by `mount` call
	super_block_t *(*get_sb)(struct s_file_system *fs, int flags,
							const char *dev, const char *opts);
							///< Initialization and superblock retrieval method
	void (*kill_sb)(super_block_t *sb); ///< Superblock free method
} file_system_t;

/**
 *	An instance of a mounted file system in the VFS
 *
 *	This structure is the entries in the fs table. It provides an entry point
 *	to the file system as well as the drivers in the superblock
 */
typedef struct s_vfsmount {
	pathname_t mountpoint; ///< Path of mountpoint
	super_block_t *sb; ///< Superblock
	int open_count; ///< Number of opened files. Used to detect open files on umount
} vfsmount_t;

/**
 *	Register a new type of file system driver
 *
 *	@param fs: the inflated `file_system_t`, containing the name of the file
 *			   system and the driver `get_sb` method
 *	@return 0 on success, or the negative of an errno on failure
 */
int fstab_register_fs(file_system_t *fs);

/**
 *	Remove a file system driver from the installed file system list
 *
 *	@param name: the name of the file system to be removed
 *	@return 0 on success, or the negative of an errno on failure
 */
int fstab_unregister_fs(const char *name);

/**
 *	Get the file system struct from the name
 *
 *	@param name: the name of the file system
 *	@return pointer to the file system struct, or NULL on failure (set errno)
 */
file_system_t *fstab_get_fs(const char *name);

/**
 *	Lookup a path in the fs table. Finds the file system driver for a specified
 *	path.
 *
 *	@note This function uses string prefix comparison to find the driver. It
 *		  does not check the actually existence of the path in the file system
 *		  or the directory access permissions.
 *
 *	@param path: the path
 *	@param offset: return value: the position in the path where the path in the
 *				   file system begins. E.g., lookup on `/dev/stdin` will return
 *				   5, which is the substring `stdin`.
 *	@return the `vfsmount_t` for the mounted file system containing the path, or
 *			or NULL on failure (set errno).
 */
vfsmount_t *fstab_get_mountpoint(const char *path, int *offset);

// System calls
/**
 *	System call handler for `mount`: mount a file system
 *
 *	@param typeaddr: the name of the file system type of this mount
 *	@param destaddr: the path of the mount point
 *	@param optaddr: other options, including device path and mount options
 *	@return 0 on success, or the negative of an errno on failure
 */
int syscall_mount(int typeaddr, int destaddr, int optaddr);

/**
 *	System call handler for `umoun`: unmount a file system
 *
 *	@param targetaddr: the mount point path to unmount
 *	@return 0 on success, or the negative of an errno on failure
 */
int syscall_umount(int targetaddr, int, int);

#endif
