/**
 *	@file fsdriver.h
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

#define BLOCK_SIZE              4096		///< size of a single block in mp3 file system
#define FILENAME_LEN            32			///< max filename length
#define DENTRY_SIZE             64			///< size of a dentry
#define FSYS_MAX_FILE           16			///< maximum number of test file
#define ITOA_BUF_SIZE           10			///< buffer size for itoa function


/**
 *	Dentry type used for file system parsing
 */
typedef struct fsys_dentry {
	int8_t filename[FILENAME_LEN];		///< filename
    int32_t filetype;					///< file type
    int32_t inode_num;					///< corresponding inode number
    int8_t reserved[24];				///< reserved bytes
} fsys_dentry_t;

/**
 *	Inode struct used for file system parsing
 */
typedef struct fsys_inode{
    int32_t length;						///< file length
    int32_t data_block_num[1023];		///< data block indices
} fsys_inode_t;

/**
 *	bootblock struct used for file system parsing
 */
typedef struct fsys_bootblock{
    int32_t dir_count;					///< total number of dentries
    int32_t inode_count;				///< total number of inode
    int32_t data_count;					///< total number of data block
    int8_t reserved[52];				///< reserved bytes
    fsys_dentry_t direntries[63];		///< dentries
} bootblock_t;

/**
 *	file operation struct used for file system testing
 */
typedef struct fsys_fops{
    int32_t (*read)(int32_t fd, void* buf, int32_t nbytes);		///< file read function
    int32_t (*write)(int32_t fd, void* buf, int32_t nbytes);	///< file write function
    int32_t (*open)(const uint8_t* filename);					///< file open function
    int32_t (*close)(int32_t fd);								///< file close function
} fsys_fops_t;

/**
 *	File strut used for file system testing
 */
typedef struct fsys_file{
    uint32_t inode;						///< inode number
    uint32_t open_count;				///< number of open file
    uint32_t pos;						///< offset file position
    fsys_fops_t *fop;					///< file operations
} fsys_file_t;


/**
 *	Global variable of the starting address of boot block
 */
extern int32_t boot_start_addr;

/**
 *	Read and fill dentry of corresponding file name
 *
 *	@param fname: name of the file to read
 *	@param dentry: the dentry that we fill in
 *	@return 0 on success, -1 for error
 */
int32_t read_dentry_by_name (const uint8_t* fname, fsys_dentry_t* dentry);


/**
 *	Read and fill dentry according to an index
 *
 *	@param index: index of the dentry to find
 *	@param dentry: the dentry that we fill in
 *	@return 0 on success, -1 for error
 */
int32_t read_dentry_by_index (uint32_t index, fsys_dentry_t* dentry);


/**
 *	Read length bytes of data in file inode, starting from offset and store into buf
 *
 *	@param inode: inode number of the file to read data from
 *	@param offset: location in file that we start reading
 *	@param buf: buffer that we read data into
 *	@param length: number of bytes read
 *	@return number of bytes read on success, -1 for error
 */
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

/**
 *	File system file read function
 *
 *	@param fd: index into file descriptor table
 *	@param buf: buffer that we read data into
 *	@param nbytes: number of bytes to read
 *	@return number of bytes read on success, -1 for error
 */
int32_t file_read(int32_t fd, void* buf, int32_t nbytes);

/**
 *	File system file write function
 *
 *	@note the function will do nothing and return -1 because
 *         the file system is read only
 */
int32_t file_write(int32_t fd, void* buf, int32_t nbytes);

/**
 *	File system file open function
 *
 *	Open the file filename, find an available fd for the 
 *	file and initialize the file structure
 *
 *	@param filename: the name of the file to open
 *	@return 0 on success for now, -1 for failure
 */
int32_t file_open(const uint8_t* filename);

/**
 *	File system file close function
 *
 *	Close the file pointed to by fd
 *
 *	@param fd: fd of the file to close
 *	@return 0 on success, -1 for failure
 */
int32_t file_close(int32_t fd);

/**
 *	File system directory read function
 *
 *	Read a filename of a file in the directory, multiple calls of read will
 *	read consecutive file names
 *
 *	@param fd: index into file descriptor table
 *	@param buf: buffer that we read data into
 *	@param nbytes: number of bytes to read
 *	@return number of bytes read on success, -1 for error
 */
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes);

/**
 *	File system directory write function
 *
 *	@note the function will do nothing and return -1 because
 *         the file system is read only
 */
int32_t dir_write(int32_t fd, void* buf, int32_t nbytes);

/**
 *	File system directory open function
 *
 *	Open a directory file, find an available fd for the 
 *	file and initialize in the file structure
 *
 *	@param filename: the name of the directory file to open
 *	@return 0 on success for now, -1 for failure
 */
int32_t dir_open(const uint8_t* filename);

/**
 *	File system directory close function
 *
 *	Close the directory file pointed to by fd
 *
 *	@param fd: fd of the file to close
 *	@return 0 on success, -1 for failure
 */
int32_t dir_close(int32_t fd);

#endif