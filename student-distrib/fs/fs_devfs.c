#include "fs_devfs.h"

#include "../lib.h"

#define DEVFS_INODE_LIMIT 32
#define DEVFS_DRIVER_LIMIT 256

static super_operations_t devfs_s_op;
static inode_operations_t devfs_inode_op;

static file_operations_t devfs_f_op;

//static inode_t	devfs_root_inode;

//static dentry_t devfs_root_dentry;
static int devfs_root_inode_ino;

static struct s_super_block devfs_sb;

static dev_driver_t devfs_table[DEVFS_DRIVER_LIMIT];

static int devfs_table_index = 0;

void devfs_installfs() {
	//int i;
	file_system_t devfs;

	//inflate s_op i_op struct
	devfs_s_op.alloc_inode = &devfs_s_op_alloc_inode;
	devfs_s_op.destroy_inode = &devfs_s_op_destroy_inode;
	devfs_s_op.write_inode = &devfs_s_op_write_inode;
	devfs_s_op.drop_inode = &devfs_s_op_drop_inode;

	devfs_inode_op.lookup = &devfs_i_op_lookup;
	devfs_inode_op.readlink = &devfs_i_op_readlink;

	devfs_f_op.open = &devfs_f_op_open;
	devfs_f_op.release = &devfs_f_op_release;
	devfs_f_op.read = NULL;
	devfs_f_op.write = NULL;
	devfs_f_op.readdir = &devfs_f_op_readdir;

	//inflate devfs in sup
	strcpy(devfs.name,"devfs");

	devfs.get_sb = &devfs_get_sb;

	devfs.kill_sb = &devfs_kill_sb;

	fstab_register_fs(&devfs);
}

struct s_super_block * devfs_get_sb(struct s_file_system *fs, int flags,
									const char *dev,const char *opts) {
	int i; //iterator
	if ((devfs_root_inode_ino = devfs_get_free_inode_num()) == -1){
		errno = ENFILE;
		return NULL;
	}
	//inflate root inode
	devfs_inode_array[devfs_root_inode_ino].ino = devfs_root_inode_ino;
	devfs_inode_array[devfs_root_inode_ino].file_type = FTYPE_DIRECTORY;
	devfs_inode_array[devfs_root_inode_ino].open_count	= 1;
	//mode to be determined
	devfs_inode_array[devfs_root_inode_ino].sb = &devfs_sb;
	devfs_inode_array[devfs_root_inode_ino].f_op = &devfs_f_op;
	devfs_inode_array[devfs_root_inode_ino].i_op = &devfs_inode_op;
	devfs_inode_array[devfs_root_inode_ino].private_data = NULL;

	devfs_root_inode_ino = devfs_root_inode_ino;

	//inflate root dentry
	strcpy(devfs_dentry_array[devfs_root_inode_ino].filename,"/dev");
	devfs_dentry_array[devfs_root_inode_ino].inode = &devfs_inode_array[devfs_root_inode_ino];

	//inflate superblock
	devfs_sb.fstype = fstab_get_fs("devfs");

	devfs_sb.s_op = &devfs_s_op;

	devfs_sb.root = &devfs_dentry_array[devfs_root_inode_ino];

	// initialize inode array
	for (i=0;i<DEVFS_INODE_LIMIT;++i){
		devfs_inode_array[i].open_count = 0;
		devfs_inode_array[i].i_op = &devfs_inode_op;
	}
	return &devfs_sb;
}

void devfs_kill_sb(){
	return;
}

struct s_inode* devfs_s_op_alloc_inode(struct s_super_block *sb){
	return -ENOSYS;
}

int devfs_get_free_inode_num(){
	int i;

	for (i = 0; i < DEVFS_INODE_LIMIT; i++) {
		if (!devfs_table[i].name[0]){
			return i;
		}
	}
	return -ENFILE;
}

void devfs_s_op_destroy_inode(inode_t* inode){
	return;
}

int devfs_s_op_write_inode(){
	//TODO
	return -ENOSYS;
}
int devfs_s_op_drop_inode(){
	//TODO
	return -ENOSYS;
}

inode_t *devfs_i_op_lookup(inode_t* inode,const char* path){
	// path should be directly the driver name
	// and inode should be the dev inode

	int i, j;
	inode_t* new_inode;
	dentry_t ret;

	for (i = 0; i < DEVFS_DRIVER_LIMIT; i++) {
		if (strncmp(path, devfs_table[i].name, VFS_FILENAME_LEN) == 0) {
			ret.ino = i;

		}
	}
	return NULL;
}

int devfs_i_op_readlink(dentry_t* dentry, char* buf, int size){
	// no symlink support
	return -ENOSYS;
}

int devfs_register_driver(const char* name, file_operations_t *ops){
	int i, idx = -1;
	inode_t *inode;

	if (!name || !ops || strlen(name) > VFS_FILENAME_LEN) {
		return -EINVAL;
	}

	for (i = 0; i < DEVFS_DRIVER_LIMIT; i++) {
		if (idx < 0 && devfs_table[i].name[0] == '\0') {
			idx = i;
		} else if (strncmp(devfs_table[i].name, name, VFS_FILENAME_LEN) == 0) {
			return -EEXISTS;
		}
	}
	if (idx < 0) {
		return -ENFILE;
	}

	strcpy(devfs_table[idx].name, name);

	inode = &(devfs_table[idx].inode);
	inode->ino = idx;
	inode->file_type = FTYPE_DEVICE;
	inode->open_count = 0;
	inode->sb = &devfs_sb;
	inode->f_op = ops;
	inode->i_op = &devfs_inode_op;

	return 0;
}

int devfs_unregister_driver(const char* name){
	int i;

	for (i = 0; i < DEVFS_DRIVER_LIMIT; i++) {
		if (strncmp(devfs_table[i].name, name, VFS_FILENAME_LEN) == 0)
			break;
	}
	if (i == DEVFS_DRIVER_LIMIT) {
		return -ENOENT;
	}
	devfs_table[i].name[0] = '\0';
	return 0;
}

int devfs_f_op_open(inode_t* inode, file_t* file){
	// This should not be called, driver's f_op will handle instead
	return -ENOSYS;
}

int devfs_f_op_release(inode_t* inode, file_t* file){
	// This should not be called, driver's f_op will handle instead
	return -ENOSYS;
}

int devfs_f_op_readdir(file_t* file, struct dirent* dirent){
	int i;
	for (i = dirent->index + 1; i < DEVFS_DRIVER_LIMIT; i++) {
		if (devfs_table[i].name[0])
			break;
	}
	if (i >= DEVFS_DRIVER_LIMIT) {
		return -ENOENT;
	}
	dirent->ino = i;
	strcpy(dirent->dentry.filename, devfs_table[file->pos].name);
	dirent->index = i;
	return 0;
}
