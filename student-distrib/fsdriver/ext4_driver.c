/*#include "../../lwext4/lwext4/ext4.c"
#include "../../lwext4/lwext4/ext4_balloc.c"
#include "../../lwext4/lwext4/ext4_fs.c"
#include "../../lwext4/lwext4/ext4_bcache.c"
#include "../../lwext4/lwext4/ext4_debug.c"
#include "../../lwext4/lwext4/ext4_hash.c"
#include "../../lwext4/lwext4/ext4_bitmap.c"
#include "../../lwext4/lwext4/ext4_dir.c"
#include "../../lwext4/lwext4/ext4_ialloc.c"
#include "../../lwext4/lwext4/ext4_block_group.c"
#include "../../lwext4/lwext4/ext4_dir_idx.c"
#include "../../lwext4/lwext4/ext4_inode.c"
#include "../../lwext4/lwext4/ext4_super.c"
#include "../../lwext4/lwext4/ext4_blockdev.c"
#include "../../lwext4/blockdev/filedev/ext4_filedev.c"*/

#include "../fs/vfs.h"
#include "ext4_driver.h"

//EXT4_BCACHE_STATIC_INSTANCE(_ext4_bcache, 8, 1024);

static inode_t ext4_inode_list[EXT4_MAX_INODE];

static pathname_t ext4_path_list[EXT4_MAX_INODE];

static ext4_file_data_t ext4_file_list[EXT4_MAX_FILE];

static ext4_dir_data_t ext4_dir_list[EXT4_MAX_DIR];

static super_block_t ext4_sb;

static file_operations_t ext4_f_op;
static inode_operations_t ext4_i_op;
static super_operations_t ext4_s_op;

int ext4_ece391_init(){
	int ret_val;
	ret_val = ext4_device_register(&_filedev, &_ext4_bcache, "hda");
	if (ret_val){
		printf("register block device failed\n");
		return -ret_val;
	}
	_ext4_init();

	// register file system
	file_system_t ext4fs;

	strcpy(ext4fs.name,"ext4fs");
	ext4fs.get_sb = &ext4_get_sb;
	ext4fs.kill_sb = &ext4_kill_sb;

	return fstab_register_fs(&ext4fs);
}

void _ext4_init(){
	int i;
	// init file operation table
	ext4_f_op.open 		= &ext4_f_op_open;
	ext4_f_op.release 	= &ext4_f_op_release;
	ext4_f_op.read		= &ext4_f_op_read;
	ext4_f_op.write		= &ext4_f_op_write;
	ext4_f_op.llseek 	= &ext4_f_op_llseek;
	ext4_f_op.readdir 	= &ext4_f_op_readdir;
	ext4_f_op.ioctl		= NULL;

	// inode operation table
	ext4_i_op.lookup 		= &ext4_i_op_lookup;
	ext4_i_op.readlink 		= &ext4_i_op_readlink;
	ext4_i_op.permission	= NULL;
	ext4_i_op.create		= &ext4_i_op_create;
	ext4_i_op.link			= NULL;
	ext4_i_op.unlink		= &ext4_i_op_unlink;
	ext4_i_op.symlink		= &ext4_i_op_symlink;
	ext4_i_op.mkdir 		= &ext4_i_op_mkdir;
	ext4_i_op.rmdir 		= &ext4_i_op_rmdir;
	ext4_i_op.truncate 		= &ext4_i_op_truncate;

	// superblock operation
	ext4_s_op.alloc_inode 	= NULL;
	ext4_s_op.open_inode 	= &ext4_s_op_open_inode;
	ext4_s_op.free_inode 	= NULL;
	ext4_s_op.read_inode 	= NULL;
	ext4_s_op.write_inode 	= &ext4_s_op_write_inode;
	ext4_s_op.drop_inode 	= NULL;

	// initialize static buffers for inodes & pathname list
	for (i=0;i<EXT4_MAX_INODE;++i){
		ext4_inode_list[i].open_count = 0;
		ext4_inode_list[i].sb = &ext4_sb;
		ext4_inode_list[i].f_op = &ext4_f_op;
		ext4_inode_list[i].i_op = &ext4_i_op;
		ext4_inode_list[i].private_data = (int)(&ext4_path_list[i]);
	}

	// initialize static buffers for files
	for (i=0;i<EXT4_MAX_FILE;++i){
		ext4_file_list[i].present = 0;
	}

	// initialize static buffers for dirs
	for (i=0;i<EXT4_MAX_DIR;++i){
		ext4_dir_list[i].present = 0;
	}
}

super_block_t* ext4_get_sb(file_system_t* fs, int flags, const char* dev, const char* opts){
	// inflate superblock for the device
	int ret_val;
	ret_val = ext4_mount("hda","/ext4");
	if (ret_val){
		printf("mount hda failed\n");
		errno = ret_val;
		return NULL;
	}

	// initialize superblock
	ext4_sb.s_op = &ext4_s_op;
	ext4_sb.fstype = fstab_get_fs("ext4fs");

	// open inode for root, it is always present
	ext4_inode_list[0].open_count = 1;

	// go get the ino number of the root
	ext4_inode_list[0].ino = EXT4_INODE_ROOT_INDEX;

	return &ext4_sb;
}

void ext4_kill_sb(super_block_t* sb){
	//ext4_unmount("/ext4");
}

int ext4_f_op_open(inode_t* inode, file_t* file){
	// find a empty space in file list
	int i;
	int temp_ret;
	for (i=0;i<EXT4_MAX_FILE;++i){
		if (!ext4_file_list[i].present){

			break;
		}
	}
	if (i>=EXT4_MAX_FILE){
		return -ENOMEM;
	}
	temp_ret = ext4_fopen2(&ext4_file_list[i].file,
		(char*)inode->private_data, file->mode);
	if (temp_ret){
		return -temp_ret;
	}
	file->private_data = (int)(&(ext4_file_list[i]));
	ext4_file_list[i].present = 1;
	return 0;
}

int ext4_f_op_release(inode_t* inode, file_t* file){
	int temp_ret;
	ext4_file_data_t* ext4_file_data = (ext4_file_data_t*)(file->private_data);
	temp_ret = ext4_fclose(&(ext4_file_data->file));

	ext4_file_data->present = 0;	// NOTE

	if (temp_ret){
		return -temp_ret;
	}

	return 0;
}

ssize_t ext4_f_op_read(file_t* file, uint8_t* buf, size_t count, off_t *offset){
	size_t rcnt;
	int temp_ret;
	ext4_file_data_t* ext4_file_data = (ext4_file_data_t*)(file->private_data);
	ext4_file* f = &(ext4_file_data->file);
	if (!ext4_file_data->present){
		return -ENOENT;
	}
	temp_ret = ext4_fseek(f, *offset, SEEK_SET);
	if (temp_ret){
		return -temp_ret;
	}
	temp_ret = ext4_fread(f, buf, count, &rcnt);
	if (temp_ret){
		return -temp_ret;
	}
	*offset = (size_t)(f->fpos);
	return (int)rcnt;
}

ssize_t ext4_f_op_write(file_t* file, uint8_t* buf, size_t count, off_t *offset){
	int temp_ret;
	size_t rcnt;
	ext4_file_data_t* ext4_file_data = (ext4_file_data_t*)(file->private_data);
	ext4_file* f = &(ext4_file_data->file);
	if (!ext4_file_data->present){
		return -ENOENT;
	}
	temp_ret = ext4_fseek(f, *offset, SEEK_SET);
	if (temp_ret){
		return -temp_ret;
	}
	temp_ret = ext4_fwrite(f, buf, count, &rcnt);
	if (temp_ret){
		return -temp_ret;
	}
	*offset = (size_t)(f->fpos);
	return (int)rcnt;
}

off_t ext4_f_op_llseek(file_t* file, off_t offset, int whence){
	int temp_ret;
	ext4_file_data_t* ext4_file_data = (ext4_file_data_t*)(file->private_data);
	ext4_file* f = &(ext4_file_data->file);
	if (!ext4_file_data->present){
		return -ENOENT;
	}
	temp_ret = ext4_fseek(f, offset, whence);
	if (temp_ret){
		return -temp_ret;
	}
	return (off_t)f->fpos;
}

int ext4_f_op_readdir(file_t* file, struct dirent* dirent){
	int i;
	const ext4_direntry *temp_ret;
	if (dirent->index == -1){
		// means new iteration
		for (i=0;i<EXT4_MAX_DIR;++i){
			if (!ext4_dir_list[i].present){
				break;
			}
		}
		if (i>=EXT4_MAX_DIR) return -ENOMEM;
		ext4_dir_list[i].present = 1;
		memcpy(&ext4_dir_list[i].dir.f, (ext4_file*)(file->private_data), sizeof(ext4_file));
		ext4_dir_list[i].dir.next_off = 0;
		dirent->ino = ((ext4_file*)(file->private_data))->inode;
		dirent->index = 0;
		dirent->data = (int)(&ext4_dir_list[i]);
	}
	temp_ret = ext4_dir_entry_next(&(((ext4_dir_data_t*)(dirent->data))->dir));
	// end of iteration
	if (temp_ret == NULL){
		// release dir buffer
		((ext4_dir_data_t*)(dirent->data))->present = 0;
		return -ENOENT;
	}
	// copy filename
	dirent->index++;
	dirent->ino = temp_ret->inode;
	strcpy((int8_t*)dirent->filename, (int8_t*)temp_ret->name);
	return 0;
}

int _ext4_inode_put(pathname_t dir, ino_t ino){
	// find next empty spot in inode list
	int i;
	for (i=0;i<EXT4_MAX_INODE;++i){
		if (!ext4_inode_list[i].open_count){
			// found one
			break;
		}
	}
	if (i >= EXT4_MAX_INODE)	return -ENOMEM;
	ext4_inode_list[i].ino = ino;
	strcpy((char*)ext4_inode_list[i].private_data, dir);
	ext4_inode_list[i].open_count++;
	return 0;
}

ino_t ext4_i_op_lookup(inode_t* inode, const char* filename){
	int temp_ret;
	ino_t ino;
	ext4_file file;
	// get current dir from the inode
	char* i_dir = (char*)(inode->private_data);
	// append filename
	temp_ret = path_cd(i_dir, filename);
	if (temp_ret){
		return temp_ret;
	}
	temp_ret = ext4_fopen2(&file, i_dir, O_RDONLY);
	if (temp_ret){
		return -temp_ret;
	}

	ino = file.inode;
	temp_ret = _ext4_inode_put(i_dir, ino); 	//TODO
	if (temp_ret){
		ino = temp_ret;
	}
	ext4_fclose(&file);
	return ino;
}

int ext4_i_op_readlink(inode_t *inode, char* buf){
	int temp_ret;
	size_t rcnt = 0;
	// open file corresponding to that inode, and read buf data
	ext4_file file;
	// get current dir from the inode
	char* i_dir = (char*)(inode->private_data);
	temp_ret = ext4_fopen2(&file, i_dir, O_RDONLY);
	if (temp_ret){
		return -temp_ret;
	}
	temp_ret = ext4_fread(&file, buf, PATH_MAX_LEN, &rcnt);
	if (temp_ret || (!rcnt)){
		return -temp_ret;
	}
	ext4_fclose(&file);
	return rcnt;
}

// permission not implemented

int ext4_i_op_create(inode_t* inode, const char* filename, mode_t mode){
	//just do oepn
	int temp_ret;
	// open file corresponding to that inode, and read buf data
	ext4_file file;
	// get current dir from the inode
	char* i_dir = (char*)(inode->private_data);
	temp_ret = ext4_fopen2(&file, i_dir, mode);
	if (temp_ret){
		return -temp_ret;
	}
	/*temp_ret = _ext4_inode_put(i_dir, file.inode);
	if (temp_ret){
		ext4_fclose(&file);
		ext4_i_op_unlink(inode, filename);
		return temp_ret;
	}*/
	ext4_fclose(&file);
	return temp_ret;
}
// link not implemented yet

int ext4_i_op_unlink(inode_t *inode, const char* filename){
	int temp_ret;
	// get current dir from the inode
	char* i_dir = (char*)(inode->private_data);
	// append filename
	temp_ret = path_cd(i_dir, filename);
	if (temp_ret){
		return temp_ret;
	}
	temp_ret = ext4_fremove(i_dir);
	if (temp_ret){
		return -temp_ret;
	}
	return temp_ret;
}

int ext4_i_op_symlink(inode_t* inode, const char* filename, const char* link){
	int temp_ret;
	// get current dir from the inode
	char* i_dir = (char*)(inode->private_data);
	// append filename
	temp_ret = path_cd(i_dir, filename);
	if (temp_ret){
		return temp_ret;
	}
	temp_ret = ext4_fsymlink(i_dir, link);
	if (temp_ret){
		return -temp_ret;
	}
	return temp_ret;
}

int ext4_i_op_mkdir(inode_t* inode, const char* filename, mode_t mode){
	int temp_ret;
	// get current dir from the inode
	char* i_dir = (char*)(inode->private_data);
	// append filename
	temp_ret = path_cd(i_dir, filename);
	if (temp_ret){
		return temp_ret;
	}
	temp_ret = ext4_dir_mk(i_dir);
	if (temp_ret){
		return -temp_ret;
	}
	temp_ret = ext4_mode_set(i_dir, mode);
	if (temp_ret){
		ext4_dir_rm(i_dir);
		return -temp_ret;
	}
	return temp_ret;
}

int ext4_i_op_rmdir(inode_t* inode, const char* filename){
	int temp_ret;
	// get current dir from the inode
	char* i_dir = (char*)(inode->private_data);
	// append filename
	temp_ret = path_cd(i_dir, filename);
	if (temp_ret){
		return temp_ret;
	}
	temp_ret = ext4_dir_rm(i_dir);
	if (temp_ret){
		return -temp_ret;
	}
	return temp_ret;
}

int ext4_i_op_truncate(inode_t* inode){
	int temp_ret;
	// open file corresponding to that inode, and read buf data
	ext4_file file;
	// get current dir from the inode
	char* i_dir = (char*)(inode->private_data);
	temp_ret = ext4_fopen2(&file, i_dir, O_RDONLY);
	if (temp_ret){
		return -temp_ret;
	}
	temp_ret = ext4_ftruncate(&file, inode->size);
	if (temp_ret){
		return -temp_ret;
	}
	return temp_ret;
}

inode_t* ext4_s_op_open_inode(super_block_t* sb, ino_t ino){
	// find in static inode pool
	int i,temp_ret;
	for (i=0;i<EXT4_MAX_INODE;++i){
		if (ext4_inode_list[i].ino == ino){
			// found
			break;
		}
	}
	// not found
	if (i>=EXT4_MAX_INODE){
		errno = EINVAL;
		return NULL;
	}

	// inflate the inode data from disk
	struct ext4_inode temp_real_inode;
	uint32_t temp_ino;
	temp_ret = ext4_raw_inode_fill((char*)(ext4_inode_list[i].private_data),
		&temp_ino, &temp_real_inode);
	// fill related fields
	_ext4_fill_inode(&ext4_inode_list[i], &temp_real_inode);
	return &ext4_inode_list[i];
	// NOTE open count increase in lookup
}

void _ext4_fill_inode(inode_t* vinode,struct ext4_inode* einode){
	// set file type
	switch(einode->mode & EXT4_INODE_MODE_TYPE_MASK){
		case EXT4_INODE_MODE_DIRECTORY:
			vinode->file_type = FTYPE_DIRECTORY;
			break;
		case EXT4_INODE_MODE_CHARDEV:
			vinode->file_type = FTYPE_DEVICE;
			break;
		case EXT4_INODE_MODE_BLOCKDEV:
			vinode->file_type = FTYPE_DEVICE;
			break;
		case EXT4_INODE_MODE_SOFTLINK:
			vinode->file_type = FTYPE_SYMLINK;
			break;
		default:
			vinode->file_type = FTYPE_REGULAR;
	}
	// set size
	vinode->size = einode->size_lo;
	// link num
	vinode->link_count = einode->links_count;
	// permission
	vinode->perm = einode->mode & (~EXT4_INODE_MODE_TYPE_MASK);
	vinode->uid = einode->uid;
	vinode->gid = einode->gid;
	vinode->atime = einode -> atime_extra;
	vinode->mtime = einode -> mtime_extra;
	vinode->ctime = einode -> ctime_extra;
}

// free inode not implemented
int ext4_s_op_free_inode(inode_t* inode){
	// decrease open count, that's it
	if (inode->open_count <= 0)
		return -EINVAL;
	else
		inode->open_count--;

	return 0;
}
// read inode not implemented

int ext4_s_op_write_inode(inode_t* inode){
	int temp_ret;
	// only mode and owner changeable
	char* i_dir = (char*)(inode->private_data);

	temp_ret = ext4_mode_set(i_dir, inode->perm);
	if (temp_ret){
		return -temp_ret;
	}
	temp_ret = ext4_owner_set(i_dir, inode->uid, inode->gid);
	if (temp_ret){
		return -temp_ret;
	}
	return temp_ret;
}

// drop inode not implemented


