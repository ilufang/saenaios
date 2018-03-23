/**
 *	@file file_lookup.h
 *
 *	Directory & dentry object functions
 */
#ifndef	DENTRY_FUNC_H
#define	DENTRY_FUNC_H

#include "vfs.h"
#include "fstab.h"

typedef struct s_nameidata{
	dentry_t* 		dentry;
	vfsmount_t*		mnt;

	unsigned int 	depth;
	char*			path;		
} nameidata_t;


//int dentry_open( ,file_system_t* file_system, int access_mode);

int path_lookup(const char* pathname, int flags, nameidata_t* nameidata);

int find_dentry(nameidata_t* nd);

inode_t* file_lookup(pathname_t path);
#endif
