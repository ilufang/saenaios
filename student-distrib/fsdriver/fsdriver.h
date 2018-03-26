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

#include "../fs/vfs.h"
#include "../errno.h"
#include "../fs/fstab.h"
#include "../../libc/include/dirent.h"

#define BLOCK_SIZE              4096		///< size of a single block in mp3 file system
#define FILENAME_LEN            32			///< max filename length
#define DENTRY_SIZE             64			///< size of a dentry
#define FSYS_MAX_FILE           16			///< maximum number of test file
#define ITOA_BUF_SIZE           10			///< buffer size for itoa function
#define MP3FS_MAX_FILE_NUM      64          ///< maximum number of files in mp3fs, root dir included

/**
 *	Dentry type used for file system parsing
 */
typedef struct fsys_dentry {
	int8_t filename[FILENAME_LEN];		///< filename
    int32_t filetype;					///< file type
    int32_t inode_num;					///< corresponding inode number
    int8_t reserved[24];				///< reserved bytes
} mp3fs_dentry_t;

/**
 *	Inode struct used for file system parsing
 */
typedef struct fsys_inode{
    int32_t length;						///< file length
    int32_t data_block_num[1023];		///< data block indices
} mp3fs_inode_t;

/**
 *	bootblock struct used for file system parsing
 */ 
typedef struct fsys_bootblock{
    int32_t dir_count;					///< total number of dentries
    int32_t inode_count;				///< total number of inode
    int32_t data_count;					///< total number of data block
    int8_t reserved[52];				///< reserved bytes
    struct fsys_dentry direntries[63];		///< dentries
} mp3fs_bootblock_t;

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
int32_t read_dentry_by_name (const uint8_t* fname, struct fsys_dentry* dentry);


/**
 *	Read and fill dentry according to an index
 *
 *	@param index: index of the dentry to find
 *	@param dentry: the dentry that we fill in
 *	@return 0 on success, -1 for error
 */
int32_t read_dentry_by_index (uint32_t index, struct fsys_dentry* dentry);


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


//functions below connect with vfs
int mp3fs_installfs();

super_block_t* mp3fs_get_sb(file_system_t* fs,int flags, const char *dev,const char *opts);

void mp3fs_kill_sb();

inode_t* _mp3fs_fetch_inode(ino_t ino);

inode_t* mp3fs_s_op_alloc_inode(super_block_t* sb);

inode_t* mp3fs_s_op_open_inode(super_block_t* sb, ino_t ino);

int mp3fs_s_op_free_inode(inode_t* inode);

int mp3fs_s_op_read_inode(inode_t* inode);

int mp3fs_s_op_write_inode(inode_t* inode);

int mp3fs_s_op_drop_inode(inode_t* inode);

ino_t mp3fs_i_op_lookup(inode_t* inode, const char* path);

int mp3fs_i_op_readlink(inode_t* inode, char* buf);

/**
 *	File system file open function
 *
 *	Open the file filename, find an available fd for the 
 *	file and initialize the file structure
 *
 *	@param filename: the name of the file to open
 *	@return 0 on success for now, -1 for failure
 */
int mp3fs_f_op_open(struct s_inode *inode, struct s_file *file);

/**
 *	File system file close function
 *
 *	Close the file pointed to by fd
 *
 *	@param fd: fd of the file to close
 *	@return 0 on success, -1 for failure
 */
int mp3fs_f_op_close(struct s_inode *inode, struct s_file *file);
/**
 *  File system file read function
 *
 *  @param fd: index into file descriptor table
 *  @param buf: buffer that we read data into
 *  @param nbytes: number of bytes to read
 *  @return number of bytes read on success, -1 for error
 */
ssize_t mp3fs_f_op_read(struct s_file *file, uint8_t *buf, size_t count,
                            off_t *offset);
/**
 *  File system file write function
 *
 *  @note the function will do nothing and return -1 because
 *         the file system is read only
 */
ssize_t mp3fs_f_op_write(struct s_file *file, uint8_t *buf, size_t count,
                            off_t *offset);
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
int mp3fs_f_op_readdir(struct s_file *file, struct dirent *dirent);

/**
 *	File system directory write function
 *
 *	@note the function will do nothing and return -1 because
 *         the file system is read only
 */
//int32_t dir_write(int32_t fd, void* buf, int32_t nbytes);


#endif
