/* filesysdriver.c - file system driver
 *	vim:ts=4 noexpandtab
 */

#include "fsdriver.h"


int32_t read_dentry_by_name (const uint8_t* fname, fsys_dentry_t* dentry){
    // ptr to bootblock
    bootblock_t * boot_ptr = (bootblock_t *)boot_start_addr;
    // total num of dentry
    int32_t dentry_count = boot_ptr->dir_count;
    // ptr to dentry array
    fsys_dentry_t* dentry_arr = (fsys_dentry_t *)(boot_ptr->direntries);
    // iteration var
    int i = 0;
    uint32_t len = strlen((int8_t*)fname);
    // iterate through all dentries
    for(i = 0; i < dentry_count; i++){
        fsys_dentry_t curr_dentry = dentry_arr[i];
        // found matchin string
        if(strncmp((int8_t*)fname, (int8_t*)curr_dentry.filename, len) == 0){
            (void)memcpy(dentry, &curr_dentry, DENTRY_SIZE);
            return 0;
        }
    }
    
    return -1;
}

int32_t read_dentry_by_index (uint32_t index, fsys_dentry_t* dentry){
    // ptr to bootblock
    bootblock_t * boot_ptr = (bootblock_t *)boot_start_addr;
    // total num of dentry
    int32_t dentry_count = boot_ptr->dir_count;
    // check for invalid dentry index
    if(index >= dentry_count || index < 0){
        return -1;
    }
    fsys_dentry_t* dentry_arr = (fsys_dentry_t *)(boot_ptr->direntries);
    (void)memcpy(dentry, &(dentry_arr[index]), DENTRY_SIZE);
    return 0;
}


int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length){
    // ptr to bootblock
    bootblock_t * boot_ptr = (bootblock_t *)boot_start_addr;
    // total num of inode
    int32_t ind_count = boot_ptr->inode_count;
    // total num of data block
    int32_t dblock_count = boot_ptr->data_count;
    // pointer to inode of target file
    fsys_inode_t * target_inode = (fsys_inode_t*)(boot_start_addr + BLOCK_SIZE * (inode + 1));
    // size of target file
    uint32_t target_file_len = (uint32_t)(target_inode->length);
    // check for invalid inode idx
    if(ind_count <= inode || inode < 0){
        return -1;
    }
    // check if end of file is reached or passed
    if(target_file_len <= offset){
        return 0;
    }
    // ptr to start of data block
    uint8_t * data_block_ptr = (uint8_t*)(boot_start_addr + BLOCK_SIZE * (ind_count + 1));
    //uint8_t * data_block_ptr = (uint8_t*)0x450000;
    // length copied
    int read_len = 0;
    // length remaining to read
    int rem_len = length < (target_file_len - offset) ? length : (target_file_len - offset);
    // index of current data block
    uint32_t block_idx = offset / BLOCK_SIZE;
    // check for invalid data block
    if(block_idx >= dblock_count){
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
        cpy_src = (uint8_t*)(data_block_ptr + target_inode->data_block_num[block_idx++]);
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


// dir syscall handlers that can't really work (or maybe they can)

int32_t dir_open(const uint8_t* filename){
    return 0;
}

int32_t dir_close(int32_t fd){
    return 0;
}

int32_t dir_read(int32_t fd, void* buf, int32_t nbytes){
    int i = 0;
    int32_t bytesread = 0;
    fsys_dentry_t dir_entry;
    while(read_dentry_by_index (i, &dir_entry) == 0){
        int8_t* fname = dir_entry.filename;
        printf(fname);
        printf("\n");
        uint32_t namelen = strlen((int8_t*)fname);
        memcpy((int8_t*)buf, (int8_t*)fname, namelen);
        buf += 32;
        bytesread += namelen;
        i++;
    }
    return bytesread;
}

int32_t dir_write(int32_t fd, void* buf, int32_t nbytes){
    return -1;
}

// file syscall handlers that can't really work

int32_t file_open(const uint8_t* filename){
    return 0;
}

int32_t file_close(int32_t fd){
    return 0;
}

int32_t file_read(int32_t fd, void* buf, int32_t nbytes){
    /*
    uint32_t inode = task_list.files[fd].inode;
    uint32_t file_loc = task_list.files[fd].pos;
    return read_data(inode, file_loc, (uint8_t*)buf, (uint32_t)nbytes);
    */
    return 0;
}

int32_t file_write(int32_t fd, void* buf, int32_t nbytes){
    return -1;
}


void test_read_file(int8_t* filename){
    clear();
    fsys_dentry_t test_dentry;
    read_dentry_by_name((uint8_t*)filename, &test_dentry);
    bootblock_t * boot_ptr = (bootblock_t *)boot_start_addr;
    // total num of data block
    int32_t dblock_count = boot_ptr->data_count;
    // pointer to inode of target file
    fsys_inode_t * target_inode = (fsys_inode_t*)(boot_start_addr + BLOCK_SIZE * (test_dentry.inode_num + 1));
    int32_t file_len = target_inode->length;
    uint8_t content[file_len];
    read_data(test_dentry.inode_num, 0, content, file_len);
    printf((int8_t*)content);
}

void test_read_dir(){
    clear();
    int32_t fd, nbytes;
    int8_t* file_names[1388];
    (void)dir_read(fd, file_names, nbytes);
}

