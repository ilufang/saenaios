/**
 *	@file fs/devfs.h
 *
 *	The devfs virtual filesystem, usually mounted on `/dev`.
 *
 *	Devfs provides VFS interface to access connected devices and drivers/
 */
#ifndef FS_DEVFS_H
#define FS_DEVFS_H

#include "fstab.h"
#include "vfs.h"

#include "../errno.h"
#include "../../libc/include/dirent.h"

typedef struct s_dev_driver{
	char name[VFS_FILENAME_LEN + 1];
	inode_t inode;
} dev_driver_t;

void devfs_installfs();

struct s_super_block * devfs_get_sb(struct s_file_system *fs, int flags, const char *dev,const char *opts);

void devfs_kill_sb();

struct s_inode* devfs_s_op_alloc_inode(struct s_super_block *sb);

int devfs_get_free_inode_num();

void devfs_s_op_destroy_inode(inode_t* inode);

int devfs_s_op_write_inode();

int devfs_s_op_drop_inode();

int devfs_i_op_readlink(dentry_t* dentry, char* buf, int size);

dentry_t devfs_i_op_lookup(inode_t* inode, const char* path);

int devfs_register_driver(const char* name, file_operations_t *ops);

int devfs_unregister_driver(const char* name);

int devfs_register_driver(const char* name, file_operations_t* ops);
void devfs_unregister_driver(const char* name);

int devfs_f_op_open(struct s_inode *inode, struct s_file *file);
int devfs_f_op_release(struct s_inode *inode, struct s_file *file);
void devfs_f_op_readdir(struct s_file *, struct dirent *dirent);

#endif
