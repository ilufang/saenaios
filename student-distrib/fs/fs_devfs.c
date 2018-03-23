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

static inode_t devfs_inode_array[DEVFS_INODE_LIMIT];

static dentry_t devfs_dentry_array[DEVFS_INODE_LIMIT];

static dev_driver_t devfs_table[DEVFS_DRIVER_LIMIT];

static int devfs_table_index = 0;

void devfs_installfs() {
	//int i;
	file_system_t devfs;

	//inflate s_op i_op struct
	devfs_s_op . alloc_inode = &devfs_s_op_alloc_inode;
	devfs_s_op . destroy_inode = &devfs_s_op_destroy_inode;
	devfs_s_op . write_inode = &devfs_s_op_write_inode;
	devfs_s_op . drop_inode = &devfs_s_op_drop_inode;

	devfs_inode_op . lookup = &devfs_i_op_lookup;
	devfs_inode_op . readlink = &devfs_i_op_readlink;

	devfs_f_op . open = &devfs_f_op_open;
	devfs_f_op . release = &devfs_f_op_release;
	devfs_f_op . read = NULL;
	devfs_f_op . write = NULL;
	devfs_f_op . readdir = &devfs_f_op_readdir;

	//inflate devfs in sup
	strcpy("devfs", devfs.name);

	devfs.get_sb = &devfs_get_sb;

	devfs.kill_sb = &devfs_kill_sb;

	fstab_register_fs(&devfs);
}

struct s_super_block * devfs_get_sb(struct s_file_system *fs, int flags, const char *dev,const char *opts) {
	int i; //iterator
	if ((devfs_root_inode_ino = get_free_inode_num())==-1){
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
	strcpy("dev",devfs_dentry_array[devfs_root_inode_ino].filename);
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
	//TODO or actually nothing
}

struct s_inode* devfs_s_op_alloc_inode(struct s_super_block *sb){
	int i = get_free_inode_num();
	if (i==-1){
		errno = ENFILE;
		return NULL;
	}
	devfs_inode_array[i].ino = i;
	// initialize the new inode
	// TODO mode
	devfs_inode_array[i].sb = &devfs_sb;
	devfs_inode_array[i].f_op = NULL;
	devfs_inode_array[i].i_op = &devfs_inode_op;
	devfs_inode_array[i].open_count = 1;
	devfs_inode_array[i].private_data = NULL;

	// actually initialize the corresponding dentry
	devfs_dentry_array[i].inode = &devfs_inode_array[i];
	return &devfs_inode_array[i];
}

int get_free_inode_num(){
	int i=0;
	while (i<DEVFS_INODE_LIMIT){
		if (devfs_inode_array[i].open_count == 0){
			return i;
		}
	}
	return -1;
}

void devfs_s_op_destroy_inode(inode_t* inode){
	inode -> open_count = 0;
}

int devfs_s_op_write_inode(){
	//TODO
	return -1;
}
int devfs_s_op_drop_inode(){
	//TODO
	return -1;
}

dentry_t* devfs_i_op_lookup(inode_t* inode,const char* path){
	//path should be directly the driver name
	//and inode should be the dev inode
	
	int i,j; //iterator
	inode_t* new_inode;
	for (i=0; i<devfs_table_index; ++i){
		//if name found in driver list
		if (!(strncmp(path, devfs_table[i].name, devfs_table[i].name_length))){
			// search for existing dentry list
			for (j=0; j<DEVFS_INODE_LIMIT; ++j){
				if (devfs_inode_array[j].open_count){
					if (!(strncmp(path, devfs_dentry_array[j].filename,devfs_table[i].name_length))){
						//found existing
						//TODO check mode
						devfs_dentry_array[j].inode->open_count++;
						return &devfs_dentry_array[j];
					}
				}
			}
			if (j>=devfs_table_index){
				//allocate a new inode and dentry 
				new_inode = devfs_s_op_alloc_inode(&devfs_sb);
				// inflate that with corresponding driver functions
				new_inode -> file_type = FTYPE_DEVICE;
				// TODO new_inode -> mode
				// note here that the type of pointer points to different size of struct
				new_inode -> f_op = (file_operations_t*)devfs_table[i].driver_op;
				// i_op already initialized during allocation
				// private data NULL
				// copy device driver name
				strcpy(devfs_table[i].name, devfs_dentry_array[new_inode->ino].filename);
				return &devfs_dentry_array[new_inode->ino];
			}
		}
	}
	return NULL;
}

int devfs_i_op_readlink(dentry_t* dentry, char* buf, int size){
	// dev does not support symlink I guess
	return -1;
}

int devfs_register_driver(const char* name, file_operations_t *ops){
	if (!name) return -1;
	if (!ops) return -1;
	if (devfs_table_index > DEVFS_DRIVER_LIMIT) return -1;

	strcpy(name, devfs_table[devfs_table_index].name);

	devfs_table[devfs_table_index].name_length = 0;

	while(*name){
		++devfs_table[devfs_table_index].name_length;
		++name;
	};

	devfs_table[devfs_table_index].driver_op = ops;

	++devfs_table_index;	
	return 1;
}

void devfs_unregister_driver(const char* name){
	//DO nothing for now
}

int devfs_f_op_open(inode_t* inode, file_t* file){
	if (inode->ino != devfs_root_inode_ino){
		errno = EINVAL;
		return -errno;
	}
	file->inode = inode;
	file->open_count = 1; //TO CHECK
	file->mode = inode->mode;
	file->pos = 0;	//TODO
	file->f_op = &devfs_f_op;
}

int devfs_f_op_release(inode_t* inode, file_t* file){
	// notice 
	devfs_s_op_destroy_inode(inode);
	return 0;
}

void devfs_f_op_readdir(file_t* file, dirent_t* dirent){
	if (file->pos >= DEVFS_DRIVER_LIMIT){
		errno = ENFILE;
		return;
	}
	dirent->dirent.index = file->pos;
	dirent->dirent.data = NULL;
	strcpy(devfs_table[file->pos].name, dirent -> dentry. filename);
	file->pos ++;
}
