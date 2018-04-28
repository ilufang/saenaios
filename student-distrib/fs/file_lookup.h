/**
 *	@file file_lookup.h
 *
 *	Directory & dentry object functions
 */
#ifndef	DENTRY_FUNC_H
#define	DENTRY_FUNC_H

#include "vfs.h"
#include "fstab.h"
#include "../lib.h"

/**
 *	struct for nameidata
 *
 *	nameidata is only used in path finding,
 *	it stores the result from last path find, and thus the source for
 *	next path finding
 *
 */
typedef struct s_nameidata{
	inode_t*		inode;	///<inode for the result/source of path find
	vfsmount_t*		mnt;	///<mounted fs for lookup

	unsigned int 	depth;	///<depth of symbolic link
	char*			path;	///<string of the path to lookup
} nameidata_t;

/**
 *	do path find for the given fd
 *
 *	go until reaching the end of the path or a error
 *
 *	@param nd: nameidata to start finding
 *	@return	0 on success, -errno for error
 *	@note still need fix for permission checking
 */
int file_find(nameidata_t* nd);

/**
 *	look up a file for the path given, and return the inode of the found file
 *
 *	the path should be a absolute path, then this function find the
 *	fs corresponding to the path, and lookup the path to find the file
 *
 *	@param path: string of the path of the file to lookup
 * 	@return the inode pointer of the file found, or the negative of an errno if
 *			the file could not be found or the caller has insufficient
 *			permissions.
 */
inode_t* file_lookup(pathname_t path);

/**
 *	Check access permission for inode with given identity
 *
 *	@param inode: the inode to be accessed
 *	@param uid: user ID
 *	@param gid: group ID
 *	@param mask: the requested permissions (see masks in libc sys/stat.h)
 *	@return 0 if access granted, or the negative of an errno if access denied
 */
int file_permission(inode_t *inode, uid_t uid, gid_t gid, int mask);

#endif
