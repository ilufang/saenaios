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

inode_t* devfs_s_op_alloc_inode(struct s_super_block *sb);

inode_t* devfs_s_op_open_inode(super_block_t* sb, ino_t ino);

int devfs_s_op_free_inode(inode_t* inode);

int devfs_s_op_read_inode();

int devfs_s_op_write_inode();

int devfs_s_op_drop_inode();

ino_t devfs_i_op_lookup(inode_t* inode,const char* path);

int devfs_i_op_readlink(inode_t* inode, char* buf);

int devfs_register_driver(const char* name, file_operations_t *ops);

int devfs_unregister_driver(const char* name);

int devfs_f_op_open(inode_t* inode, file_t* file);

int devfs_f_op_release(inode_t* inode, file_t* file);

int devfs_f_op_readdir(file_t* file, struct dirent* dirent);

#endif
