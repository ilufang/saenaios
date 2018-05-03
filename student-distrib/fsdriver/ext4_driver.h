#ifndef EXT4_DRIVER_H_
#define EXT4_DRIVER_H_

#include <stdlib.h>
#include "ext4_lwext4_include.h"
#include "../fs/vfs.h"
#include "../fs/fstab.h"
#include <dirent.h>
#include "../errno.h"

#define EXT4_MAX_INODE 64
#define EXT4_MAX_FILE 64
#define EXT4_MAX_DIR 8



typedef struct s_ext4_file_data{
	int8_t present;
	ext4_file file;
} ext4_file_data_t;

typedef struct s_ext4_dir_data{
	int8_t present;
	ext4_dir dir;
} ext4_dir_data_t;

int ext4_ece391_init();

void _ext4_init();

super_block_t* ext4_get_sb(file_system_t* fs, int flags, const char* dev, const char* opts);

void ext4_kill_sb(super_block_t*);

int ext4_f_op_open(inode_t* inode, file_t* file);

int ext4_f_op_release(inode_t* inode, file_t* file);

ssize_t ext4_f_op_read(file_t* file, uint8_t* buf, size_t count, off_t *offset);

ssize_t ext4_f_op_write(file_t* file, uint8_t* buf, size_t count, off_t *offset);

off_t ext4_f_op_llseek(file_t* file, off_t offset, int whence);

int ext4_f_op_readdir(file_t* file, struct dirent* dirent);

int _ext4_inode_put(pathname_t dir, ino_t ino);

ino_t ext4_i_op_lookup(inode_t* inode, const char* filename);

int ext4_i_op_readlink(inode_t *inode, char* buf);

int ext4_i_op_create(inode_t* inode, const char* filename, mode_t mode);

int ext4_i_op_unlink(inode_t *inode, const char* filename);

int ext4_i_op_symlink(inode_t* inode, const char* filename, const char* link);

int ext4_i_op_mkdir(inode_t* inode, const char* filename, mode_t mode);

int ext4_i_op_rmdir(inode_t* inode, const char* filename);

int ext4_i_op_truncate(inode_t* inode);

inode_t* ext4_s_op_open_inode(super_block_t* sb, ino_t ino);

void _ext4_fill_inode(inode_t* vinode, struct ext4_inode* einode);

int ext4_s_op_free_inode(inode_t* inode);

int ext4_s_op_write_inode(inode_t* inode);

#endif
