/**
 *	@file keyboard.h
 *
 *	Header file for filesystem driver
 *
 *	vim:ts=4 noexpandtab
 */

#ifndef _FSDRIVER_H
#define _FSDRIVER_H

#include "../types.h"
#include "../i8259.h"
#include "../lib.h"

#define INODE_NUM_OFFSET        4
#define DATA_BLOCK_NUM_OFFSET   8
#define BLOCK_SIZE              4096
#define FILENAME_LEN            32
#define DENTRY_SIZE             64
#define FSYS_MAX_FILE           16
#define ITOA_BUF_SIZE           10


// dentry type used for file system parsing
typedef struct fsys_dentry {
	int8_t filename[FILENAME_LEN];
    int32_t filetype;
    int32_t inode_num;
    int8_t reserved[24];
} fsys_dentry_t;

// inode struct used for file system parsing
typedef struct fsys_inode{
    int32_t length;
    int32_t data_block_num[1023];
} fsys_inode_t;

// bootblock struct used for file system parsing
typedef struct fsys_bootblock{
    int32_t dir_count;
    int32_t inode_count;
    int32_t data_count;
    int8_t reserved[52];
    fsys_dentry_t direntries[63];
} bootblock_t;

// file operation struct used for file system testing
typedef struct fsys_fops{
    int32_t (*read)(int32_t fd, void* buf, int32_t nbytes);
    int32_t (*write)(int32_t fd, void* buf, int32_t nbytes);
    int32_t (*open)(const uint8_t* filename);
    int32_t (*close)(int32_t fd);
} fsys_fops_t;

// file strut used for file system testing
typedef struct fsys_file{
    uint32_t inode;
    uint32_t open_count;
    uint32_t pos;
    fsys_fops_t *fop;
} fsys_file_t;



extern int32_t boot_start_addr;

int32_t read_dentry_by_name (const uint8_t* fname, fsys_dentry_t* dentry);
int32_t read_dentry_by_index (uint32_t index, fsys_dentry_t* dentry);
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

int32_t file_read(int32_t fd, void* buf, int32_t nbytes);
int32_t file_write(int32_t fd, void* buf, int32_t nbytes);
int32_t file_open(const uint8_t* filename);
int32_t file_close(int32_t fd);

int32_t dir_read(int32_t fd, void* buf, int32_t nbytes);
int32_t dir_write(int32_t fd, void* buf, int32_t nbytes);
int32_t dir_open(const uint8_t* filename);
int32_t dir_close(int32_t fd);

#endif
