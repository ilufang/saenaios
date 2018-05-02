/**
 *	@file fsdriver/mp3fs_driver.h
 *
 *	Header file for filesystem driver
 */
#ifndef FSDRIVER_MP3FS_DRIVER_H
#define FSDRIVER_MP3FS_DRIVER_H

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
#define MP3FS_MAX_FILE_NUM      65          ///< maximum number of files in mp3fs, root dir included

#define MP3FS_TEMP_BASE		100	///< I-no base for temporary file
#define MP3FS_TEMP_MAX		64	///< Max number of temporary files


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
int mp3fs_installfs(int32_t bootblock_start_addr);

/**
 *  inflate the superblock object for mp3fs
 *
 *  initialize superblock object for mp3fs and initialize device table & inodes
 *
 *  @param  fs: file system object
 *  @param  flags: flags, not used for now
 *  @param  dev: not used
 *  @param  opts: not used
 */
super_block_t* mp3fs_get_sb(file_system_t* fs,int flags, const char *dev,const char *opts);

/**
 *  work around rtc inode number for mp3fs
 */
void mp3fs_brutal_magic();

/**
 *  close a superblock object
 */
void mp3fs_kill_sb();

/**
 *  private helper function to fetch inode data from mp3fs disk
 *
 *  @param ino: inode number of inode to be fetched
 *  @return inode pointer on success, NULL on failure
 */
inode_t* _mp3fs_fetch_inode(ino_t ino);

/**
 *  Create a new i-node with new i-number in the file system.
 *
 *  Similar to the principle of `creat`. The driver owns the memory of the
 *  i-node, but the user must free it with `free_inode` after using it.
 *
 *  @param sb: the super block of the file system
 *  @return pointer to a new index node
 */
inode_t* mp3fs_s_op_alloc_inode(super_block_t* sb);

/**
 *  Retrieve an i-node from the file system with the given i-number.
 *
 *  Similar to the principle of `open`. The driver owns the memory of the
 *  i-node, but the user must free it with `free_inode` after using it.
 *
 *  @param sb: the super block of the file system
 *  @param ino: the i-number of the i-node to be opened
 *  @return pointer to a populated i-node with the specified `ino`
 */
inode_t* mp3fs_s_op_open_inode(super_block_t* sb, ino_t ino);

/**
 *  Release an i-node
 *
 *  Similar to the principle of `close`. The driver should handle any
 *  clean-up when the user finishes using the i-node. The i-node passed in
 *  must come from either `alloc_inode` or `open_inode`.
 *
 *  @param inode: the inode pointer from `alloc_inode` or `open_inode`
 *  @return 0 on success, or the negative of an errno on failure
 */
int mp3fs_s_op_free_inode(inode_t* inode);

/**
 *  Populate/update an i-node
 *
 *  The driver reads the `ino` field of the i-node passed in and populates
 *  the rest of the fields.
 *
 *  @param inode: the i-node with field `ino` specified. The rest of the
 *                fields will be modified by this call
 *  @return 0 on success, or the negative of an errno on failure
 */
int mp3fs_s_op_read_inode(inode_t* inode);

/**
 *  Flush an i-node to permanent storage
 *
 *  The driver write the metadata fields in the i-node to the permanent
 *  index node on its permanent storage specified by the `ino` field. The
 *  driver may or may not choose to flush certain fields according to its
 *  file system features or specification.
 *
 *  @param inode: the i-node to write
 *  @return 0 on success, or the negative of an errno on failure
 */
int mp3fs_s_op_write_inode(inode_t* inode);

/**
 *  Removes an i-node from permanent storage
 *
 *  The driver removes the index node on its permanent storage specified by
 *  the `ino` field. For instance, when all hardlinks to a file has been
 *  removed, `drop_inode` would be called to permanently delete the file and
 *  free up the corresponding disk space.
 *
 *  The driver also performs clean-up on the i-node structure. No further
 *  call to `free_inode` is necessary or allowed.
 *
 *  @param inode: the i-node to delete
 *  @return 0 on success, or the negative of an errno on failure
 */
int mp3fs_s_op_drop_inode(inode_t* inode);

/**
 *  Find file with name in directory
 *
 *  @param inode: current inode, must be a directory
 *  @param filename: the name to lookup
 *  @return the i-number of the file, or the negative of an errno on failure
 */
ino_t mp3fs_i_op_lookup(inode_t* inode, const char* path);

/**
 *  Read symbolic link of inode
 *
 *  @param inode: current inode, must be a symbolic link
 *  @param buf: the buffer to read the linked path into, must be big enough
 *              to hold a path (PATH_MAX_LEN+1)
 */
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
 *	Create directory
 *
 *	This function will create an empty temporary directory for a mountpoint. The
 *	newly created directory cannot hold files, and will not be saved after power
 *	off
 *
 *	@param inode: the root directory
 *	@param filename: the name of the new directory
 */
int mp3fs_i_op_mkdir(struct s_inode *inode, const char *filename, mode_t mode);

/**
 *	Add symlink to MP3FS temporary file list
 *
 *	@parma filename: the name of the symlink
 *	@param link: content of the symlink
 */
int mp3fs_symlink(const char *filename, const char *link);

/**
 *	Add directory to MP3FS temporary file list
 *
 *	@param filename: name of the directory
 *	@param mode: permissions of the directory
 */
int mp3fs_mkdir(const char *filename, mode_t mode);


#endif
