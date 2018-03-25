/**
 *	@file fs/fs_devfs.h
 *
 *	The devfs virtual filesystem, usually mounted on `/dev`.
 *
 *	Devfs provides VFS interface to access connected devices and drivers/
 */
#ifndef FS_DEVFS_H
#define FS_DEVFS_H

#include "fstab.h"
#include "vfs.h"

#include "../errno.h"
#include "../../libc/include/dirent.h"

/**
 *	A struct for entries of the device driver table
 *
 */
typedef struct s_dev_driver{
	char name[VFS_FILENAME_LEN + 1];	///<driver name
	inode_t inode;						///<inode entity for the driver
} dev_driver_t;

/**
 *	install devfs to the kernel
 *
 *	initialize get_sb,s_op i_op f_op and install file system information
 */
int devfs_installfs();

/**
 *	inflate the superblock object for devfs
 *
 *	initialize superblock object for devfs and initialize device table & inodes
 *
 *	@param	fs: file system object
 *	@param 	flags: flags, not used for now
 *	@param	dev: not used
 *	@param	opts: not used
 */
struct s_super_block * devfs_get_sb(struct s_file_system *fs, int flags, const char *dev,const char *opts);

/**
 *	kill the superblock object
 *
 *	do nothing for now
 */
void devfs_kill_sb();

/**
 *	allocate space for a new inode object
 *
 *	do nothing for now, and should not be called
 *
 *	@param sb: superblock object
 *	@return a allocated inode
 *
 *	@note would return a not implemented fault
 */
inode_t* devfs_s_op_alloc_inode(struct s_super_block *sb);

/**
 *	open a inode object for the give inode number
 *
 *	@param	sb: superblock object
 *	@param  ino: ino number of the inode expected
 *	@return opened inode corresponding to the inode number given
 *
 *	@note it also create space for the inode returned
 */
inode_t* devfs_s_op_open_inode(super_block_t* sb, ino_t ino);

/**
 *	free space for a inode object
 *
 *	declaring the the inode is out of use for the caller
 *
 *	@param inode: inode to be freed
 *	@return 0 on success or errno number on error
 *
 *	@note for the driver, it first decrease the open count,
 *		then if it goes to zero, free it,
 *		and actually not freed for devfs since we have no malloc!
 */
int devfs_s_op_free_inode(inode_t* inode);

/**
 *	read inode and inflate the inode
 *
 *	The driver reads the `ino` field of the i-node passed in and populates
 *	the rest of the fields.
 *
 *	@param inode: inode object to be filled
 *	@return 0 on success errno on error
 *
 *	@note not meaningful for devfs
 */
int devfs_s_op_read_inode(inode_t* inode);

/**
 *	write inode to the disk
 *
 *	The driver write the metadata fields in the i-node to the permanent
 *	index node on its permanent storage specified by the `ino` field. The
 *	driver may or may not choose to flush certain fields according to its
 *	file system features or specification.
 *
 *	@param inode: inode object to be filled
 *	@return 0 on success errno on error
 *
 *	@note not meaningful for devfs
 */
int devfs_s_op_write_inode(inode_t* inode);

/**
 *	Removes an i-node from permanent storage
 *
 *	The driver removes the index node on its permanent storage specified by
 *	the `ino` field. For instance, when all hardlinks to a file has been
 *	removed, `drop_inode` would be called to permanently delete the file and
 *	free up the corresponding disk space.
 *
 *	The driver also performs clean-up on the i-node structure. No further
 *	call to `free_inode` is necessary or allowed.
 *
 *	@param inode: the i-node to delete
 *	@return 0 on success, or the negative of an errno on failure
 *	@note not meaningful for devfs
 */
int devfs_s_op_drop_inode(inode_t* inode);

/**
 *	Find file with name in directory
 *
 *	@param inode: current inode, must be a directory
 *	@param path: the name to lookup
 *	@return the i-number of the file, or the negative of an errno on failure
 */
ino_t devfs_i_op_lookup(inode_t* inode,const char* path);

/**
 *	Read symbolic link of inode
 *
 *	@param inode: current inode, must be a symbolic link
 *	@param buf: the buffer to read the linked path into, must be big enough
 *				to hold a path (PATH_MAX_LEN+1)
 *	@note not meaningful for devfs
 */
int devfs_i_op_readlink(inode_t* inode, char* buf);

/**
 *	register a driver to the devfs
 *
 *	@param name: device driver name
 *	@param ops: operations for the driver
 *	@return 0 on success -errno on error
 */
int devfs_register_driver(const char* name, file_operations_t *ops);

/**
 *	unregister a driver from the devfs
 *
 *	@param name: device driver name to be unregistered
 *	@return 0 on success -errno on error
 */
int devfs_unregister_driver(const char* name);

/**
 *	Initialize a file for open
 *
 *	The caller will fill in default values for all fields except
 *	`private_data` prior to calling this function. This function should not
 *	modify them if unnecessary (especially `open_count`). This function
 *	should initialize the `private_data` field if it needs to. This function
 *	can also set a special f_op for the file if necessary.
 *
 *	@param inode: the i-node the file is opened from
 *	@param file: the newly-opened file
 *	@return 0 on success, or the negative of an errno on failure
 *	@note this is for /dev only
 */
int devfs_f_op_open(inode_t* inode, file_t* file);

/**
 *	Clean-up an opened file for close
 *
 *	Similar to `open`, this function should only release its `private_data`
 *	if set during `open`. The caller will release and rely on the other
 *	fields for clean-up.
 *
 *	@param inode: the i-node the file is opened from
 *	@param file: the file to be closed
 *	@return 0 on success, or the negative of an errno on failure
 */
int devfs_f_op_release(inode_t* inode, file_t* file);

/**
 *	Read the directory file for dir entires.
 *
 *	`dirent` will be used to return data as well as track progress. For the
 *	first iteration, the `index` field of `dirent` must contain -1. For
 *	subsequent reads, the `index` field will contain the index of the last
 *	visited index. Index does not have to be consecutive, but the same index
 *	must read out the same dir entry. Driver may also utilize the `data`
 *	field to store private data, but it will not be notified of the deletion
 *	of `dirent`.
 *
 *	@param file: the file to iterate, must be a directory
 *	@param dirent: the dirent, whose `index` field is used to track
 *				   iteration progress. The output dir entry will be written
 *				   to the `ino` and `filename` field.
 *	@return 0 on success, or the negative of an errno on failure.
 *			Specifically, -ENOENT when iteration has finished
 *	@note for devfs, /dev is the only directory that should be called
 */
int devfs_f_op_readdir(file_t* file, struct dirent* dirent);

#endif
