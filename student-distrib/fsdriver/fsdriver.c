/* filesysdriver.c - file system driver
 *	vim:ts=4 noexpandtab
 */

#include "fsdriver.h"



int32_t boot_start_addr;
// temporary file array used for testing
fsys_file_t * test_fd_arr[FSYS_MAX_FILE];

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
    //if(len > FILENAME_LEN) len = FILENAME_LEN;
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


int32_t file_open(const uint8_t* filename){
    int i = 0;
    fsys_dentry_t dentry;
    // filename does not exist
    if(read_dentry_by_name(filename, &dentry) == -1){
        return -1;
    }
    // create and fill in file struct
    fsys_file_t file;
    file.inode = dentry.inode_num;
    file.open_count++;
    file.pos = 0;
    fsys_fops_t f_ops;
    f_ops.open = &file_open;
    f_ops.close = &file_close;
    f_ops.read = &file_read;
    f_ops.write = &file_write;
    // find an available fd
    for(i = 0; i < FSYS_MAX_FILE; i++){
        if(!test_fd_arr[i]){
            test_fd_arr[i] = &file;
            return 0;
        }
    }
    // if there's not available fd, return -1
    return -1;
}

int32_t file_close(int32_t fd){
    // free the corresponding fd
    test_fd_arr[fd] = NULL;
    return 0;
}

int32_t file_read(int32_t fd, void* buf, int32_t nbytes){
    // use file array and fd to get inode and offset information
    uint32_t inode = test_fd_arr[fd]->inode;
    uint32_t file_loc = test_fd_arr[fd]->pos;
    uint32_t bytes_read = read_data(inode, file_loc, (uint8_t*)buf, (uint32_t)nbytes);
    // update file offset position for successful read
    if(bytes_read != -1){
        test_fd_arr[fd]->pos += bytes_read;
    }
    return bytes_read;
}

int32_t file_write(int32_t fd, void* buf, int32_t nbytes){
    // return -1 for a read-only file system
    return -1;
}

int32_t dir_open(const uint8_t* filename){
    // same as file open?
    return file_open((uint8_t*)".");
}

int32_t dir_close(int32_t fd){
    return 0;
}

int32_t dir_read(int32_t fd, void* buf, int32_t nbytes){
    int32_t bytesread = 0;
    fsys_dentry_t dir_entry;
    fsys_file_t *dirfile = test_fd_arr[fd];
    uint32_t readpos = dirfile->pos;
    bootblock_t * boot_ptr = (bootblock_t *)boot_start_addr;
    // total num of dentry
    int32_t dentry_count = boot_ptr->dir_count;
    // check for invalid dentry index
    if(readpos>=dentry_count) return 0;
    if(read_dentry_by_index (readpos, &dir_entry) == 0){
        // read dentry and print information, update offset of corresponding file
        fsys_inode_t * target_inode = (fsys_inode_t*)(boot_start_addr + BLOCK_SIZE * (dir_entry.inode_num + 1));
        // make sure that filename length doesn't exceed 32 chars
        uint32_t namelen = strlen((int8_t*)dir_entry.filename);
        if(namelen>FILENAME_LEN) namelen = FILENAME_LEN;
        // clear buffer
        memset((int8_t*)buf, '\0', FILENAME_LEN);
        // copy filename into buffer
        memcpy((int8_t*)buf, (int8_t*)dir_entry.filename, namelen);
        // print file information
        int32_t file_len = target_inode->length;
        // terminal_print("FILE NAME: ");
        // terminal_print(buf);
        terminal_print("FILE SIZE: ");
        int8_t len_str_buf[ITOA_BUF_SIZE];
        terminal_print(itoa(file_len, len_str_buf, 10));
        terminal_print(", FILE TYPE: ");
        int8_t type_str_buf[ITOA_BUF_SIZE];
        terminal_print(itoa(dir_entry.filetype, type_str_buf, 10));
        terminal_print(", ");
        bytesread = namelen;
        // update read position offset
        dirfile->pos = readpos+1;
        return bytesread;
    }
    // if read failed return -1
    return -1;
}

int32_t dir_write(int32_t fd, void* buf, int32_t nbytes){
    // return -1 for a read only file system
    return -1;
}

