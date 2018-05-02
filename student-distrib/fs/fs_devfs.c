#include "fs_devfs.h"

#include "../lib.h"

#define DEVFS_DRIVER_LIMIT 256	///<up limit for number of drivers

static super_operations_t devfs_s_op;
static inode_operations_t devfs_inode_op;

static file_operations_t devfs_f_op;

//static inode_t	devfs_root_inode;

//static dentry_t devfs_root_dentry;
//static int devfs_root_inode_ino;

static struct s_super_block devfs_sb;

// 'dev' directory is set to be the index DEVFS_DRIVER_LIMIT
static dev_driver_t devfs_table[DEVFS_DRIVER_LIMIT+1];

int devfs_installfs() {
	// fstable stores value, so no worry here
	file_system_t devfs;
	int i;

	if (devfs_s_op.alloc_inode) {
		return -EEXIST;
	}

	//inflate s_op i_op struct specific to this fs
	devfs_s_op.alloc_inode = &devfs_s_op_alloc_inode;
	devfs_s_op.open_inode = &devfs_s_op_open_inode;
	devfs_s_op.free_inode = &devfs_s_op_free_inode;
	devfs_s_op.read_inode = &devfs_s_op_read_inode;
	devfs_s_op.write_inode = &devfs_s_op_write_inode;
	devfs_s_op.drop_inode = &devfs_s_op_drop_inode;

	devfs_inode_op.lookup = &devfs_i_op_lookup;
	devfs_inode_op.readlink = &devfs_i_op_readlink;
	// f_op for devfs directories
	devfs_f_op.open = &devfs_f_op_open;
	devfs_f_op.release = &devfs_f_op_release;
	devfs_f_op.read = NULL;
	devfs_f_op.write = NULL;
	devfs_f_op.readdir = &devfs_f_op_readdir;

	// dev is defined to be indexed DEVFS_DRIVER_LIMIT
	strcpy(devfs_table[DEVFS_DRIVER_LIMIT].name,"/dev");
	//may need error checking?

	// inflate inode for /dev directory
	devfs_table[DEVFS_DRIVER_LIMIT].inode.ino = DEVFS_DRIVER_LIMIT;
	devfs_table[DEVFS_DRIVER_LIMIT].inode.file_type = FTYPE_DIRECTORY;
	devfs_table[DEVFS_DRIVER_LIMIT].inode.open_count = 0;
	devfs_table[DEVFS_DRIVER_LIMIT].inode.sb = &devfs_sb;
	devfs_table[DEVFS_DRIVER_LIMIT].inode.f_op = &devfs_f_op;
	devfs_table[DEVFS_DRIVER_LIMIT].inode.i_op = &devfs_inode_op;
	devfs_table[DEVFS_DRIVER_LIMIT].inode.perm = 555;
	devfs_table[DEVFS_DRIVER_LIMIT].inode.uid = 0;
	devfs_table[DEVFS_DRIVER_LIMIT].inode.gid = 0;

	//inflate devfs and register
	strcpy(devfs.name,"devfs");

	//inflate superblock
	devfs_sb.fstype = fstab_get_fs("devfs");

	devfs_sb.s_op = &devfs_s_op;

	devfs_sb.root = DEVFS_DRIVER_LIMIT;

	// initialize device table
	// this fields are the same for all device inodes here
	for (i=0;i<DEVFS_DRIVER_LIMIT;++i){
		devfs_table[i].name[0] = '\0';
		devfs_table[i].inode.open_count = 0;
		devfs_table[i].inode.ino = i;
		devfs_table[i].inode.sb = &devfs_sb;
		devfs_table[i].inode.f_op = NULL;
		devfs_table[i].inode.i_op = &devfs_inode_op;
		devfs_table[i].inode.perm = 777;
		devfs_table[i].inode.uid = 0;
		devfs_table[i].inode.gid = 0;

		//private data don't care
	}


	devfs.get_sb = &devfs_get_sb;

	devfs.kill_sb = &devfs_kill_sb;

	return fstab_register_fs(&devfs);
}

struct s_super_block * devfs_get_sb(struct s_file_system *fs, int flags,
									const char *dev,const char *opts) {
	return &devfs_sb;
}

void devfs_kill_sb(){
	// do nothing for now
	return;
}

inode_t* devfs_s_op_alloc_inode(struct s_super_block *sb){
	// this shouldn't be called
	errno = ENOSYS;
	return NULL;
}

inode_t* devfs_s_op_open_inode(super_block_t* sb, ino_t ino){
	// ino exceeds limit
	if (ino > DEVFS_DRIVER_LIMIT){
		errno = EINVAL;	// NEED CHECK
		return NULL;
	}
	if (!sb){
		errno = EINVAL;
		return NULL;
	}
	// invalid ino, i.e. driver not defined
	if (devfs_table[ino].name[0]=='\0'){
		errno = EINVAL;	// NEED CHECK
		return NULL;
	}
	devfs_table[ino].inode.open_count ++;
	return &devfs_table[ino].inode;
}

int devfs_s_op_free_inode(inode_t* inode){
	//decrease open count of inode
	if (!inode){
		return -EINVAL;
	}
	if (inode->open_count==0){
		return -ENODEV;
	}
	--inode->open_count;
	return 0;
}

int devfs_s_op_read_inode(inode_t* inode){
	// does nothing actually
	return -ENOSYS;
}

int devfs_s_op_write_inode(inode_t* inode){
	// does nothing actually
	return -ENOSYS;
}

int devfs_s_op_drop_inode(inode_t* inode){
	// does nothing actually
	return -ENOSYS;
}

ino_t devfs_i_op_lookup(inode_t* inode,const char* path){
	if (!inode){
		return -EINVAL;
	}
	if (!path){
		return -EINVAL;
	}
	// path should be directly the driver name
	// and inode should be the dev inode
	int i;

	for (i = 0; i < DEVFS_DRIVER_LIMIT; i++) {
		if (strncmp(path, devfs_table[i].name, VFS_FILENAME_LEN) == 0) {
			return i;
		}
	}
	return -ENXIO;
}

int devfs_i_op_readlink(inode_t* inode, char* buf){
	//every symbolic link linked to tty!
	strncpy(buf, "tty", 7);
	return 7;
}

int devfs_register_driver(const char* name, file_operations_t *ops){
	int i;
	inode_t *inode;
	// sanity check
	if (!name || !ops || strlen(name) > VFS_FILENAME_LEN) {
		return -EINVAL;
	}
	// find a place to register and repeated driver check
	for (i = 0; i < DEVFS_DRIVER_LIMIT; i++) {
		if (devfs_table[i].name[0] == '\0') {
			// found a empty slots
			break;
		} else if (strncmp(devfs_table[i].name, name, VFS_FILENAME_LEN) == 0) {
			// same name driver already exists
			return -EEXIST;
		}
	}
	// number of drivers exceeds limit
	if (i > DEVFS_DRIVER_LIMIT) {
		return -ENFILE;
	}

	// i represent the device table index to be register
	strcpy(devfs_table[i].name, name);

	inode = &(devfs_table[i].inode);
	// special case for stdin / stdout, they are symbolic link
	if ((!strncmp(name,"stdin",6))||(!strncmp(name,"stdout",7))||
		(!strncmp(name, "stderr", 7))){
		inode->file_type = FTYPE_SYMLINK;
	}else{
		inode->file_type = FTYPE_DEVICE;
	}

	inode->f_op = ops;
	inode->uid = 0;
	inode->gid = 0;
	inode->perm = 0777; // All granted for now

	return 0;
}

int devfs_unregister_driver(const char* name){
	int i;
	// search for entry in the table
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
	if (!inode || !file) return -EINVAL;
	file->inode = inode;
	file->pos = 0;
	file->f_op = inode->f_op;
	//not private data
	return 0;
}

int devfs_f_op_release(inode_t* inode, file_t* file){
	return 0;
}

int devfs_f_op_readdir(file_t* file, struct dirent* dirent){
	// for /dev only
	int i;
	for (i = dirent->index + 1; i < DEVFS_DRIVER_LIMIT; i++) {
		if (devfs_table[i].name[0])
			break;
	}
	if (i >= DEVFS_DRIVER_LIMIT) {
		return -ENOENT;
	}
	dirent->ino = i;
	strcpy(dirent->filename, devfs_table[i].name);
	dirent->index = i;
	return 0;
}
