/* filesysdriver.c - file system driver
 *	vim:ts=4 noexpandtab
 */

#include "mp3fs_driver.h"

#include "../boot/page_table.h"

static mp3fs_bootblock_t* boot_ptr;
static mp3fs_dentry_t* mp3fs_dentry_start_ptr;
// temporary file array used for testing
fsys_file_t * test_fd_arr[FSYS_MAX_FILE];

// static data for fsdriver
// super block object
static  super_block_t mp3fs_sb;
// inode buffer
static  inode_t mp3fs_file_table[MP3FS_MAX_FILE_NUM];

// static function tables
static super_operations_t mp3fs_s_op;
static inode_operations_t mp3fs_i_op;
static file_operations_t mp3fs_f_op;
//static int count = 0;

static char mp3fs_ftype_string[5] = "ldf";

int32_t read_dentry_by_name (const uint8_t* fname, mp3fs_dentry_t* dentry){
    // ptr to bootblock
    // bootblock_t * boot_ptr = (bootblock_t *)boot_start_addr;
    // total num of dentry
    int32_t dentry_count = boot_ptr->dir_count;
    // ptr to dentry array
    mp3fs_dentry_t* dentry_arr = (mp3fs_dentry_t *)(boot_ptr->direntries);
    // iteration var
    int i = 0;
    //uint32_t len = strlen((int8_t*)fname);
    //if(len > FILENAME_LEN) len = FILENAME_LEN;
    // iterate through all dentries
    for(i = 0; i < dentry_count; i++){
        mp3fs_dentry_t curr_dentry = dentry_arr[i];
        // found matching string
        if(strncmp((int8_t*)fname, (int8_t*)curr_dentry.filename, FILENAME_LEN) == 0){
            (void)memcpy(dentry, &curr_dentry, DENTRY_SIZE);
            return 0;
        }
    }

    return -1;
}

int32_t read_dentry_by_index (uint32_t index, mp3fs_dentry_t* dentry){
    // ptr to bootblock
    // bootblock_t * boot_ptr = (bootblock_t *)boot_start_addr;
    // total num of dentry
    int32_t dentry_count = boot_ptr->dir_count;
    // check for invalid dentry index
    if(index >= (uint32_t)dentry_count){
        return -1;
    }
    mp3fs_dentry_t* dentry_arr = (mp3fs_dentry_t *)(boot_ptr->direntries);
    (void)memcpy(dentry, &(dentry_arr[index]), DENTRY_SIZE);
    return 0;
}

int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length){
    // ptr to bootblock
    // bootblock_t * boot_ptr = (bootblock_t *)boot_start_addr;
    // total num of inode
    int32_t ind_count = boot_ptr->inode_count;
    // total num of data block
    int32_t dblock_count = boot_ptr->data_count;
    // pointer to inode of target file
    mp3fs_inode_t * target_inode = (mp3fs_inode_t*)((uint8_t*)boot_ptr + BLOCK_SIZE * (inode + 1));
    // size of target file
    uint32_t target_file_len = (uint32_t)(target_inode->length);
    // check for invalid inode idx
    if((uint32_t)ind_count <= inode){
        return -1;
    }
    // check if end of file is reached or passed
    if(target_file_len <= offset){
        return 0;
    }
    // ptr to start of data block
    uint8_t * data_block_ptr = (uint8_t*)((uint8_t*)boot_ptr + BLOCK_SIZE * (ind_count + 1));
    //uint8_t * data_block_ptr = (uint8_t*)0x450000;
    // length copied
    int read_len = 0;
    // length remaining to read
    int rem_len = length < (target_file_len - offset) ? length : (target_file_len - offset);
    // index of current data block
    uint32_t block_idx = offset / BLOCK_SIZE;
    // check for invalid data block
    if(block_idx >= (uint32_t)dblock_count){
        return -1;
    }
    // size to copy in current data block
    uint32_t n_copy;
    if(rem_len < BLOCK_SIZE){
        n_copy = rem_len;
    }
    else{
        n_copy = BLOCK_SIZE - offset % BLOCK_SIZE;
    }
    // ptr to source address of current copy operation
    uint8_t* cpy_src = (uint8_t*)(data_block_ptr +
                                  target_inode ->data_block_num[block_idx] * BLOCK_SIZE +
                                  offset % BLOCK_SIZE);
    while(rem_len > 0){
        (void)memcpy(buf, cpy_src, n_copy);
        rem_len -= n_copy;
        read_len += n_copy;
        if(rem_len <= 0){
            return read_len;
        }
        // update source and dest address for next copy
        buf += n_copy;
        cpy_src = (uint8_t*)(data_block_ptr + target_inode->data_block_num[++block_idx] * BLOCK_SIZE);
        // calculate size to copy for next iteration
        if(rem_len < BLOCK_SIZE){
            n_copy = rem_len;
        }
        else{
            n_copy = BLOCK_SIZE;
        }
    }
    return read_len;
}

// functions below connect with vfs
int mp3fs_installfs(int32_t bootblock_start_addr){
    // need to magicalize the file data

    // get address of boot block from parameter
    boot_ptr = (mp3fs_bootblock_t*)bootblock_start_addr;
    // pinpoint install position of mp3fs
    if ((bootblock_start_addr > 0x800000) && (bootblock_start_addr < 0xC00000)){
        page_dir_add_4MB_entry(bootblock_start_addr,bootblock_start_addr,
                            PAGE_DIR_ENT_PRESENT | PAGE_DIR_ENT_RDWR |
                            PAGE_DIR_ENT_SUPERVISOR | PAGE_DIR_ENT_GLOBAL);
    }
    // pinpoint install position of mp3fs
    if (!((bootblock_start_addr > 0x400000) && (bootblock_start_addr < 0x800000))){
        return -EINVAL;
    }
    mp3fs_dentry_start_ptr = (mp3fs_dentry_t*)((uint8_t*)boot_ptr + DENTRY_SIZE);

    mp3fs_brutal_magic();
    //mp3fs_inode_index = 0;

    file_system_t mp3fs;

    mp3fs_s_op.alloc_inode = &mp3fs_s_op_alloc_inode;
    mp3fs_s_op.open_inode = &mp3fs_s_op_open_inode;
    mp3fs_s_op.free_inode = &mp3fs_s_op_free_inode;
    mp3fs_s_op.read_inode = &mp3fs_s_op_read_inode;
    mp3fs_s_op.write_inode = &mp3fs_s_op_write_inode;
    mp3fs_s_op.drop_inode = &mp3fs_s_op_drop_inode;

    mp3fs_i_op.lookup = &mp3fs_i_op_lookup;
    mp3fs_i_op.readlink = &mp3fs_i_op_readlink;

    mp3fs_f_op.open = &mp3fs_f_op_open;
    mp3fs_f_op.release = &mp3fs_f_op_close;
    mp3fs_f_op.read = &mp3fs_f_op_read;
    mp3fs_f_op.write = &mp3fs_f_op_write;
    mp3fs_f_op.readdir = &mp3fs_f_op_readdir;

    strcpy(mp3fs.name, "mp3fs");

    mp3fs.get_sb = &mp3fs_get_sb;

    mp3fs.kill_sb = &mp3fs_kill_sb;

    return fstab_register_fs(&mp3fs);
}

void mp3fs_brutal_magic(){
    int i;
    for (i=0; i<boot_ptr->dir_count; ++i){
        if (strncmp(mp3fs_dentry_start_ptr[i].filename, "rtc", 3)==0){
            // a designated magic number
            mp3fs_dentry_start_ptr[i].inode_num = 64;
            break;
        }
    }
}

super_block_t* mp3fs_get_sb(struct s_file_system *fs, int flags,
                                    const char *dev,const char *opts) {
    int i; // iterator
    // initialize & inflate super_block
    mp3fs_sb.fstype = fstab_get_fs("mp3fs");

    mp3fs_sb.s_op = &mp3fs_s_op;

    // get root dentry's (i.e. root dir) inode number
    mp3fs_sb.root = mp3fs_dentry_start_ptr->inode_num;

    mp3fs_sb.private_data = MP3FS_IDENTIFIER;

    // initialize file table
    // of course brutal force table
    for (i=0;i<MP3FS_MAX_FILE_NUM;++i){
        mp3fs_file_table[i].open_count = 0;
        mp3fs_file_table[i].ino = i;
    }
    return &mp3fs_sb;
}

void mp3fs_kill_sb(){
    // do nothing for now
}

inode_t* mp3fs_s_op_alloc_inode(super_block_t* sb){
    // should not be called for now
    errno = ENOSYS;
    return NULL;
}

inode_t* mp3fs_s_op_open_inode(super_block_t* sb, ino_t ino){
    // search for the ino given
    if ((!sb) || (ino<0) || (ino>MP3FS_MAX_FILE_NUM)){
        errno = EINVAL;
        return NULL;
    }
    // search for the inodes in the memory
    if (mp3fs_file_table[ino].open_count){
        // found in inodes in memory
        ++mp3fs_file_table[ino].open_count;
        return &mp3fs_file_table[ino];
    }
    // then go for the disk although it should not reach here
    // found that in the disk, then get it from 'disk'!
    return _mp3fs_fetch_inode(ino);
}

int mp3fs_s_op_free_inode(inode_t* inode){
    // sanity check
    if ((!inode) || (inode->open_count==0)){
        return -EINVAL;
    }
    inode->open_count--;
    return 0;
}

int mp3fs_s_op_read_inode(inode_t* inode){
    // should not be called for now
    return -ENOSYS;
}

int mp3fs_s_op_write_inode(inode_t* inode){
    // should not be called for now
    return -ENOSYS;
}

int mp3fs_s_op_drop_inode(inode_t* inode){
    // should not be called for now
    return -ENOSYS;
}

inode_t* _mp3fs_fetch_inode(ino_t ino){
    int i;
    mp3fs_inode_t * target_inode;

    int32_t dentry_count = boot_ptr->dir_count;
    // check for invalid dentry index
    if(ino >= MP3FS_MAX_FILE_NUM || ino < 0){
        errno = EINVAL;
        return NULL;
    }

    for (i=0; i<dentry_count; ++i){
        if (mp3fs_dentry_start_ptr[i].inode_num == ino)
            break;
    }

    if (i >= dentry_count){
        errno = ENOENT;
        return NULL;
    }
    target_inode = (mp3fs_inode_t*)((uint8_t*)boot_ptr + BLOCK_SIZE * (ino + 1));

    // found it, ah, fetch from disk
    mp3fs_file_table[ino].file_type = mp3fs_ftype_string[mp3fs_dentry_start_ptr[i].filetype];
    mp3fs_file_table[ino].open_count = 1;   // should be 1 isn't it?
    // TODO mp3fs_file_table[ino].mode
    mp3fs_file_table[ino].sb = &mp3fs_sb;
    mp3fs_file_table[ino].i_op = &mp3fs_i_op;
    mp3fs_file_table[ino].f_op = &mp3fs_f_op;
    // private data is the length of the file
    mp3fs_file_table[ino].private_data = target_inode->length;
    return &mp3fs_file_table[ino];
}

ino_t mp3fs_i_op_lookup(inode_t* inode, const char* path){
    int temp_return;
    mp3fs_dentry_t temp_mp3fs_dentry;
    if ((!inode) || (!path)){
        return -EINVAL;
    }

    if ((temp_return = read_dentry_by_name((uint8_t*)path, &temp_mp3fs_dentry))){
        // must be error
        return -ENOENT;
    }
    // found
    return temp_mp3fs_dentry.inode_num;
}

int mp3fs_i_op_readlink(inode_t* inode, char* buf){
    // should not be called
    return -ENOSYS;
}

int mp3fs_f_op_open(struct s_inode *inode, struct s_file *file){
    // sanity check
    if ((!inode) || (!file)){
        return -EINVAL;
    }
    // inflate the file object according to the inode
    file->inode = inode;
    // TODO mode file->mode
    file->pos = 0;
    file->f_op = &mp3fs_f_op;
    // no private data
    return 0;
}

int mp3fs_f_op_close(struct s_inode *inode, struct s_file *file){
    // sanity check
    if ((!inode) || (!file))
        return -EINVAL;
    // no private data
    return 0;
}

ssize_t mp3fs_f_op_read(struct s_file *file, uint8_t *buf, size_t count,
                            off_t *offset){
    int temp_read_count;
    if (file->inode->file_type == FTYPE_DIRECTORY){
        return -EISDIR;
    }
    temp_read_count = read_data(file->inode->ino, file -> pos, buf, count);
    if (temp_read_count>=0)
        file -> pos += temp_read_count;
    return temp_read_count;
}

ssize_t mp3fs_f_op_write(struct s_file *file, uint8_t *buf, size_t count,
                            off_t *offset){
    return -EROFS;
}

int mp3fs_f_op_readdir(struct s_file *file, struct dirent *dirent){
    // sanity check
    if ((!file) || (!dirent)){
        return -EINVAL;
    }

    int i;  //iterator
    mp3fs_dentry_t temp_mp3fs_dentry;

    for (i = dirent->index + 1; i < MP3FS_MAX_FILE_NUM; ++i){
        if (!read_dentry_by_index(i, &temp_mp3fs_dentry))
            //found
            break;
    }
    if (i >= MP3FS_MAX_FILE_NUM){
        return -ENOENT;
    }
	/*
	if (!read_dentry_by_index(count, &temp_mp3fs_dentry)){
		count++;
	}
	else{
		count = 0;
        return -ENOENT;
    }*/
    dirent->ino = i;
    strncpy(dirent->filename, temp_mp3fs_dentry.filename, 32);
    dirent->index = i;
    return 0;
}
