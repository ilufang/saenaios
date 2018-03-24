#include "file_lookup.h"

#include "../errno.h"

#define temp_buff_size 256
#define sim_link_uplimit 7


inode_t* file_lookup(pathname_t path){
	vfsmount_t *fs;
	int i;	// number for path offset

	// initialize nameidata for this file lookup
	nameidata_t nd;
	// 	
	nd.depth = 0;
	nd.path = path;

	char pathtemp[temp_buff_size];
	int pathtemp_length;

file_lookup_start:
	if (!(fs = fstab_get_mountpoint(nd.path, &i))) {
		//TODO set errno
		return NULL;
	}
	// update nameidata
	nd.dentry = fs -> root;
	nd.mnt = fs;
	nd.path += i;
	nd.depth ++;

	// up limit of sim link //TODO 
	if (nd.depth > temp_buff_size){
		errno = EMLINK;
		return NULL;
	}

	if (find_dentry(&nd)) {
		return NULL;
	}

	// handle sim link
	if (nd.dentry -> inode -> file_type == FTYPE_SYMLINK){
		// update nd for sim link
		if (nd.dentry -> inode -> i_op -> readlink){
			if ((pathtemp_length = (*nd.dentry -> inode -> i_op -> readlink)(nd.dentry,pathtemp,temp_buff_size))==-1){
				//TODO errno
				return NULL;
			}
			if (pathtemp_length<temp_buff_size) pathtemp[pathtemp_length] = '\0';
			// hope it doesn't reach up limit
		}else{
			// readlink function not implemented
			//TODO set errno -ENOSYS;
			return NULL;
		}
		path_cd(path,pathtemp);
		nd.path = path;
		goto file_lookup_start;
	}

	// lookup finish, nothing to release
	return nd.dentry->inode;
}

int find_dentry(nameidata_t* nd){
	// sanity check
	dentry_t* temp_dentry;
	if (!nd){ 
		errno = EINVAL;
		return -errno;
	}
	while (*nd->path){
		if (!(temp_dentry = (*(nd->dentry->inode->i_op->lookup))(nd->dentry->inode, nd->path))){
			errno = ENOSYS;
			return -errno;
		}
		nd->dentry = temp_dentry;
		//shift path to next '\'
		while (*(nd->path) || *(nd->path)!='\\'){
			++(nd->path);
		}
		++(nd->path);
	}
	return 0;
}
