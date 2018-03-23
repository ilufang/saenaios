/**
 *	@file fs/fstab.h
 *
 *	The management of specific file systems and mounted instances of them
 */
#ifndef FS_FSTAB_H
#define FS_FSTAB_H

#include "vfs.h"

#define FSTAB_FS_NAME_LEN	16

typedef struct s_file_system {
	char name[FSTAB_FS_NAME_LEN]; ///< NameE used by `mount` call
	superblock_t *(*get_sb)(struct s_file_system *fs, int flags,
							const char *dev, const char *opts);
							///< Initialization and superblock retrieval method
	void (*kill_sb)(superblock_t *sb); ///< Superblock free method
} file_system_t;

typedef struct s_vfsmount {
	path_t mountpoint; ///< Path of mountpoint
	dentry_t *root; ///< dentry of mounted root in current FS
	superblock_t *superblock; ///< Superblock
	int count; ///< Number of opened files. Used to detect open files on umount
} vfsmount_t;

int fstab_register_fs(file_system_t *fs);

int fstab_unregister_fs(const char *name);

file_system_t *fstab_get_fs(const char *name);

vfsmount_t *fstab_get_mountpoint(const char *path, int *offset);

// System calls

struct __attribute__((__packed__)) sys_mount_opts {
	char *source;
	unsigned long mountflags;
	char *opts;
};

int syscall_mount(int typeaddr, int destaddr, int optaddr);

int syscall_umount(int targetaddr);

#endif
