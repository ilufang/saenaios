#include "file_lookup.h"

#include "../errno.h"

#define sym_link_buff_size 256
#define sym_link_uplimit 7

#define REAL_SYM_LINK

inode_t* file_lookup(pathname_t path){
	vfsmount_t *fs;
	int i;	// number for path offset

	int temp_return;
	// initialize nameidata for this file lookup
	nameidata_t nd;
	//
	nd.depth = 0;
	nd.path = path;

file_lookup_start:
	if (!(fs = fstab_get_mountpoint(nd.path, &i))) {
		// errno set in the function called
		return NULL;
	}
	// update nameidata
	nd.mnt = fs;
	// get and open the root inode
	if (!(nd.inode = (*(fs->sb->s_op->open_inode))(fs ->sb, fs -> sb -> root))){
		// root inode open failed
		errno = ENOENT;
		return NULL;
	}
	nd.path += i;
	nd.depth ++;

	// up limit of sym link
	if (nd.depth > sym_link_uplimit){
		errno = EMLINK;
		return NULL;
	}

	if ((temp_return = find_file(&nd))){
		//find file failed, errno set in function called
		errno = -temp_return;
		return NULL;
	}

	// handle sym link
	if (nd.inode -> file_type == FTYPE_SYMLINK){
		if (fs->sb->private_data != MP3FS_IDENTIFIER){
			char pathtemp[sym_link_buff_size];
			int pathtemp_length;
			//int temp_buff_size;
			// update nd for sym link
			if (nd.inode -> i_op -> readlink){
				if ((pathtemp_length = (*(nd.inode -> i_op -> readlink))(nd.inode,pathtemp))<0){
					// the errno should be set by function called
					return NULL;
				}
				if (pathtemp_length<sym_link_buff_size)
					pathtemp[pathtemp_length] = '\0';
				else{
					errno = EFBIG;
					return NULL;
				}
			}else{
				// readlink function not implemented
				errno = ENOSYS;
				return NULL;
			}
			if (path_cd(path,pathtemp)){
				// errno set in path_cd;
				errno = EINVAL;
				return NULL;
			}
			//update appended link and then restart lookup process
			nd.path = path;
			goto file_lookup_start;
		}else{
			// special treatment for ece391 mp3fs
			strcpy(path, "/dev/rtc");
			nd.path = path;
			// TODO discard previously found inode
			nd.inode->sb->s_op->free_inode(nd.inode);
			goto file_lookup_start;
		}
	}

	// lookup finish, nothing to release
	return nd.inode;
}

int find_file(nameidata_t* nd){
	// sanity check
	ino_t temp_ino;
	if (!nd){
		errno = EINVAL;
		return -errno;
	}
	while (*nd->path){
		// function existence check
		if (!(nd->inode->i_op->lookup)){
			errno = ENOSYS;
			return -errno;
		}
		// get over '/'
		if (*(nd->path) == '/') ++(nd->path);
		if ((temp_ino = (*(nd->inode->i_op->lookup))(nd->inode, nd->path)) < 0){
			//error, errno set in called function
			return temp_ino;
		}else{
			// free previous directory inode
			(*nd -> mnt -> sb -> s_op -> free_inode)(nd -> inode);
			// note: this is necessary
			nd -> inode = NULL;
			//get inode for next search
			if (!(nd -> inode = (*(nd -> mnt -> sb -> s_op -> open_inode))(nd -> mnt -> sb, temp_ino))){
				// open inode failed, errno set in called function
				return -errno;
			}

		}
		//shift path to next '/', get over it
		while ((*(nd->path)) && (*(nd->path)!='/')){
			++(nd->path);
		}
		if (*(nd->path) == '/') ++(nd->path);
	}
	return 0;
}
