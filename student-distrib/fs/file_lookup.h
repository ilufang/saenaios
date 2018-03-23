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

typedef struct s_dentry{
	int 			use_count;
	//spinlock_t		d_lock;
	inode_t*		inode;
	dentry_t*		parent_dentry;
	//subdirectory

	char*			filename;
	
	//dentry_operations*	

} dentry_t;


int filp_open();

int oepn_namei(const char* pathname, nameidata* namei);

//int dentry_open( ,file_system_t* file_system, int access_mode);

int path_lookup(const char* pathname, int flags, nameidata_t* nameidata);

#endif