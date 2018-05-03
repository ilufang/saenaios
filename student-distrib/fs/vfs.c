#include "vfs.h"

#include "../lib.h"
#include "../errno.h"

#include "file_lookup.h"
#include "../proc/task.h"

#include "../../libc/src/syscalls.h" // Definitions from libc
#include "../../libc/include/unistd.h"

file_t vfs_files[VFS_MAX_OPEN_FILES];

#define DIRENT_INDEX_AUTO	-2

int syscall_ece391_open(int pathaddr, int b, int c) {
	int ret;
	ret = syscall_open(pathaddr, O_RDWR, 0);
	if (ret < 0)
		return -1;
	if (ret >= 8) {
		syscall_close(ret, 0, 0);
		return -1;
	}
	return ret;
}

int syscall_open(int pathaddr, int flags, int mode) {
	task_t *proc;
	pathname_t path;
	int i, avail_fd = -1;
	inode_t *inode;
	file_t *file;
	int perm_mask = 0;

	proc = task_list + task_current_pid();

	strcpy(path, proc->wd);
	errno = -path_cd(path, (char *)pathaddr);
	if (errno != 0) {
		return -errno;
	}

	for (i = 0; i < TASK_MAX_OPEN_FILES; i++) {
		if (proc->files[i] == NULL) {
			avail_fd = i;
			break;
		}
	}
	if (avail_fd < 0) {
		return -EMFILE;
	}

	if (!(inode = file_lookup(path))){
		if (errno == ENOENT) {
			// Create the file if flags allow
			if (mode & O_CREAT) {
				inode = vfs_create_file(path, mode);
				if (!inode) {
					return -errno;
				}
			} else {
				return -errno;
			}
		} else {
			return -errno;
		}
	} else {
		if (mode & O_EXCL) {
			(*inode->sb->s_op->free_inode)(inode);
			return -EEXIST;
		}
	}

	if (mode & O_RDONLY) {
		perm_mask |= 4; // Read bit
	}
	if (mode & O_WRONLY) {
		perm_mask |= 2; // Write bit
	}
	errno = -file_permission(inode, proc->uid, proc->gid, perm_mask);
	if (errno != 0) {
		// Permission denied
		(*inode->sb->s_op->free_inode)(inode);
		return -errno;
	}

	if (!(file = vfs_open_file(inode, flags))) {
		return -errno;
	}

	/*errno = -(*file->f_op->open)(inode, file);
	if (errno != 0) {
		vfs_close_file(file);
		return -errno;
	}*/
	proc->files[avail_fd] = file;

	if (flags & O_TRUNC) {
		syscall_truncate(avail_fd, 0, 0);
	}

	return avail_fd;
}

int syscall_ece391_close(int fd, int b, int c) {
	int ret;
	if (fd == 0 || fd == 1) {
		// ECE391 think they shouldn't be able to close stdin and stdout
		return -1;
	}
	ret = syscall_close(fd, b, c);
	if (ret < 0)
		return -1;
	return ret;
}

int syscall_close(int fd, int b, int c) {
	task_t *proc;
	file_t *file;

	proc = task_list + task_current_pid();

	file = proc->files[fd];
	//file = curr_pcb->fd_arr[fd];
	if (!file || fd >= TASK_MAX_OPEN_FILES || fd < 0) {
		return -EBADF;
	}

	vfs_close_file(file);

	proc->files[fd] = NULL;
	return 0;
}

int syscall_ece391_read(int fd, int bufaddr, int size) {
	int ret;
	struct dirent dent;
	if (fd == 1) return -1;
	ret = syscall_read(fd, bufaddr, size);
	if (ret == -EISDIR) {
		dent.index = DIRENT_INDEX_AUTO; // Workaround ece391_read auto dir listing
		ret = syscall_getdents(fd, (int)&dent, 0);
		if (ret == 0) {
			ret = strlen((char*)dent.filename);
			if (ret > size) {
				ret = size;
			}
			strncpy((char *)bufaddr, dent.filename, ret);
		}
		if (ret == -ENOENT)
			ret = 0;
	}
	if(ret < 0) ret = -1;
	return ret;
}

int syscall_read(int fd, int bufaddr, int count) {
	task_t *proc;
	file_t *file;

	if (!bufaddr) {
		return -EFAULT;
	}

	proc = task_list + task_current_pid();

	file = proc->files[fd];
	if (!file || fd >= TASK_MAX_OPEN_FILES || fd < 0) {
		return -EBADF;
	}
	if (!(file->mode & O_RDONLY)) {
		// Not opened for reading
		return -EBADF;
	}
	if (!file->f_op->read) {
		if (file->inode->file_type == FTYPE_DIRECTORY) {
			return -EISDIR;
		}
		return -ENOSYS;
	}
	// TODO: no permission check
	return (*file->f_op->read)(file, (uint8_t *) bufaddr, count, &(file->pos));
}

int syscall_ece391_write(int fd, int bufaddr, int count) {
	int ret;
	if (fd == 0) return -1;
	ret = syscall_write(fd, bufaddr, count);
	if (ret < 0)
		return -1;
	return ret;
}

int syscall_write(int fd, int bufaddr, int count) {
	task_t *proc;
	file_t *file;

	if (!bufaddr) {
		return -EFAULT;
	}

	proc = task_list + task_current_pid();

	file = proc->files[fd];
	if (!file || fd >= TASK_MAX_OPEN_FILES || fd < 0) {
		return -EBADF;
	}
	if (!(file->mode & O_WRONLY)) {
		// Not opened for writing
		return -EBADF;
	}
	if (!file->f_op->write) {
		return -ENOSYS;
	}
	// TODO: no permission check
	return (*file->f_op->write)(file, (uint8_t *) bufaddr, count, &(file->pos));
}

int syscall_lseek(int fd, int offset, int whence) {
	task_t *proc;
	file_t *file;

	proc = task_list + task_current_pid();

	file = proc->files[fd];
	if (!file || fd >= TASK_MAX_OPEN_FILES || fd < 0) {
		return -EBADF;
	}
	if (!file->f_op->llseek) {
		switch(whence) {
			case SEEK_SET:
				file->pos = offset;
				break;
			case SEEK_CUR:
				file->pos += offset;
				break;
			case SEEK_END:
				return -ENOSYS;
			default:
				return -ENOSYS;
		}
		if ((int)file->pos < 0) {
			file->pos = 0;
		}
		return file->pos;
	} else {
		return (*file->f_op->llseek)(file, offset, whence);
	}
}

int syscall_getdents(int fd, int bufaddr, int c) {
	task_t *proc;
	file_t *file;
	struct dirent *dent;
	int ret;

	if (!bufaddr) {
		return -EFAULT;
	}

	proc = task_list + task_current_pid();

	file = proc->files[fd];
	//file = curr_pcb -> fd_arr[fd];
	if (!file || fd >= TASK_MAX_OPEN_FILES || fd < 0) {
		return -EBADF;
	}
	if (!file->f_op->readdir) {
		return -ENOSYS;
	}
	dent = (struct dirent *)bufaddr;
	if (dent->index == DIRENT_INDEX_AUTO) {
		// Workaround ece391_read auto dir listing
		dent->index = file->pos - 1;
		ret = (*file->f_op->readdir)(file, dent);
		if (ret >= 0) {
			file->pos = dent->index + 1;
		}
		return ret;
	} else
		return (*file->f_op->readdir)(file, dent);
}

file_t *vfs_open_file(inode_t *inode, int mode) {
	int i, ret;

	if (!inode) {
		errno = EFAULT;
		return NULL;
	}
	if (!(mode & (O_RDONLY | O_WRONLY))) {
		errno = EINVAL;
		return NULL;
	}

	for (i = 0; i < VFS_MAX_OPEN_FILES; i++) {
		if (!vfs_files[i].inode) {
			break;
		}
	}
	if (i == VFS_MAX_OPEN_FILES) {
		errno = ENFILE;
		return NULL;
	}

	vfs_files[i].inode = inode;
	vfs_files[i].mode = mode;
	vfs_files[i].open_count = 1;
	vfs_files[i].pos = 0;
	vfs_files[i].f_op = inode->f_op;
	ret = (*inode->f_op->open)(inode, vfs_files + i);
	if (ret != 0) {
		errno = -ret;
		vfs_files[i].inode = NULL;
		return NULL;
	}
	return vfs_files + i;
}

int vfs_close_file(file_t *file) {
	if (!file || !file->inode) {
		return -EFAULT;
	}
	file->open_count--;
	if (file->open_count != 0) {
		// Someone else is still using this file
		return 0;
	}
	// Close this file
	(*file->f_op->release)(file->inode, file);
	if (file->inode->sb->s_op->write_inode) {
		(*file->inode->sb->s_op->write_inode)(file->inode);
	}
	(*file->inode->sb->s_op->free_inode)(file->inode);
	file->inode = NULL;
	file->mode = 0;
	file->f_op = NULL;
	return 0;
}

inode_t *vfs_create_file(pathname_t path, mode_t mode) {
	char *filename;
	int i;
	inode_t *inode;
	task_t *proc;

	proc = task_list + task_current_pid();

	// Find base filename
	filename = NULL;
	for (i = 1; path[i]; i++) {
		if (path[i] == '/') {
			filename = path + i;
		}
	}
	if (!filename) {
		errno = EINVAL;
		return NULL;
	}
	*filename = '\0';
	filename++;
	if (!*filename) {
		errno = EINVAL;
		filename[-1] = '/';
		return NULL;
	}
	if (!(inode = file_lookup(path))) {
		filename[-1] = '/';
		return NULL;
	}
	errno = -file_permission(inode, proc->uid, proc->gid, S_IW);
	if (errno != 0) {
		// Permission denied
		goto cleanup;
	}

	if (!inode->i_op->mkdir) {
		errno = ENOSYS;
		goto cleanup;
	}

	errno = -(*inode->i_op->create)(inode, filename, mode);

	// Success
	(*inode->sb->s_op->write_inode)(inode);
	(*inode->sb->s_op->free_inode)(inode);
	filename[-1] = '/';
	return file_lookup(path);

cleanup:
	(*inode->sb->s_op->free_inode)(inode);
	filename[-1] = '/';
	return NULL;
}

int syscall_stat(int path, int stat_in, int c){
	int temp_return;
	int fd;
	stat_t* stat = (stat_t*)stat_in;
	task_t* proc;
	file_t* temp_file;

	if (!stat){
		return -EINVAL;
	}

	temp_return = syscall_open(path, O_RDONLY, 0);
	if (temp_return < 0){
		return temp_return;
	}

	fd = temp_return;

	proc = task_list + task_current_pid();

	temp_file = proc->files[fd];

	//TODO PERMISSION CHECK

	// fill in the stat data
	stat->st_ino = temp_file->inode->ino;
	stat->st_mode = temp_file->mode;		// NEED CHECK
	//stat->st_uid							TODO
	//stat->st_gid							TODO

	//NOTE, TEMPORARY WORKAROUND
	stat->st_size = temp_file->inode->private_data;
	//stat->st_blksize						TODO
	//stat->st_blocks						TODO

	syscall_close(fd, 0, 0);

	return 0;
}

int syscall_fstat(int fd, int stat_in, int c){
	stat_t* stat = (stat_t*)stat_in;
	task_t* proc;
	file_t* temp_file;

	if (fd < 0){
		// invalid fd
		return -EINVAL;
	}

	if (!stat){
		return -EINVAL;
	}

	proc = task_list + task_current_pid();

	temp_file = proc->files[fd];

	//TODO PERMISSION CHECK

	if (temp_file->open_count <= 0){
		// invalid fd
		return -EINVAL;
	}

	// fill in the stat data
	stat->st_ino = temp_file->inode->ino;
	stat->st_mode = temp_file->mode;		// NEED CHECK
	//stat->st_uid							TODO
	//stat->st_gid							TODO

	//NOTE, TEMPORARY WORKAROUND
	stat->st_size = temp_file->inode->private_data;
	//stat->st_blksize						TODO
	//stat->st_blocks						TODO

	return 0;
}

int syscall_lstat(int path, int stat, int c){
	return -ENOSYS;
}

int syscall_chmod(int fd, int mode, int c) {
	task_t *proc;
	file_t *file;
	inode_t *inode;

	proc = task_list + task_current_pid();

	file = proc->files[fd];
	if (!file || fd >= TASK_MAX_OPEN_FILES || fd < 0) {
		return -EBADF;
	}
	inode = file->inode;
	if (proc->uid != inode->uid && proc->uid != 0) {
		// User is not owner and user is not root
		return -EPERM;
	}
	// Perform chmod
	inode->perm = mode;
	return 0;
}

int syscall_chown(int fd, int uid, int gid) {
	task_t *proc;
	file_t *file;
	inode_t *inode;

	proc = task_list + task_current_pid();

	file = proc->files[fd];
	if (!file || fd >= TASK_MAX_OPEN_FILES || fd < 0) {
		return -EBADF;
	}
	inode = file->inode;
	if (proc->uid != inode->uid && proc->uid != 0) {
		// User is not owner and user is not root
		return -EPERM;
	}
	// Perform chown
	inode->uid = uid;
	inode->gid = gid;
	// Prevent chown setuid/setguid elevation vulnerability
	inode->perm &= ~(S_ISUID | S_ISGID);
	return 0;
}

int syscall_link(int path1p, int path2p, int c) {
	task_t *proc;
	pathname_t path;
	char *filename;
	inode_t *inode_from, *inode_to;
	int i;

	proc = task_list + task_current_pid();

	errno = task_access_memory(path1p);
	if (errno != 0) {
		return -errno;
	}
	errno = task_access_memory(path2p);
	if (errno != 0) {
		return -errno;
	}

	// Open source file
	strcpy(path, proc->wd);
	errno = -path_cd(path, (char *)path1p);
	if (errno != 0) {
		return -errno;
	}
	if (!(inode_from = file_lookup(path))){
		return -errno;
	}
	errno = -file_permission(inode_from, proc->uid, proc->gid, S_IR);
	if (errno != 0) {
		// Permission denied
		goto cleanup_from;
	}

	// Open destination file
	strcpy(path, proc->wd);
	errno = -path_cd(path, (char *)path2p);
	if (errno != 0) {
		goto cleanup_from;
	}
	// Find base filename
	filename = NULL;
	for (i = 1; path[i]; i++) {
		if (path[i] == '/') {
			filename = path + i;
		}
	}
	if (!filename) {
		errno = EINVAL;
		goto cleanup_from;
	}
	*filename = '\0';
	filename++;
	if (!*filename) {
		errno = EINVAL;
		goto cleanup_from;
	}
	if (!(inode_to = file_lookup(path))) {
		goto cleanup_from;
	}
	errno = -file_permission(inode_to, proc->uid, proc->gid, S_IW);
	if (errno != 0) {
		// Permission denied
		goto cleanup_fromto;
	}
	if (inode_from->sb != inode_to->sb) {
		// No hardlink across device
		errno = EXDEV;
		goto cleanup_fromto;
	}

	if (!inode_to->i_op->link) {
		errno = ENOSYS;
		goto cleanup_fromto;
	}

	errno = -(*inode_to->i_op->link)(inode_from->ino, inode_to, filename);

	if (errno != 0) {
		goto cleanup_fromto;
	}

	inode_from->link_count++;

	// Success
	(*inode_to->sb->s_op->write_inode)(inode_to);
	(*inode_from->sb->s_op->write_inode)(inode_from);
	errno = 0;

cleanup_fromto:
	(*inode_to->sb->s_op->free_inode)(inode_to);
cleanup_from:
	(*inode_from->sb->s_op->free_inode)(inode_from);
	return -errno;
}

int syscall_unlink(int pathp, int b, int c) {
	task_t *proc;
	pathname_t path;
	char *filename;
	inode_t *inode_from, *inode_to;
	int i;

	proc = task_list + task_current_pid();

	errno = task_access_memory(pathp);
	if (errno != 0) {
		return -errno;
	}

	// Open file to be unlinked
	strcpy(path, proc->wd);
	errno = -path_cd(path, (char *)pathp);
	if (errno != 0) {
		return -errno;
	}
	if (!(inode_from = file_lookup(path))){
		return -errno;
	}
	// Find base filename
	filename = NULL;
	for (i = 1; path[i]; i++) {
		if (path[i] == '/') {
			filename = path + i;
		}
	}
	if (!filename) {
		errno = EINVAL;
		goto cleanup_from;
	}
	*filename = '\0';
	filename++;
	if (!*filename) {
		errno = EINVAL;
		goto cleanup_from;
	}

	// Open containing directory
	if (!(inode_to = file_lookup(path))) {
		goto cleanup_from;
	}
	errno = -file_permission(inode_to, proc->uid, proc->gid, S_IW);
	if (errno != 0) {
		// Permission denied
		goto cleanup_fromto;
	}

	if (!inode_to->i_op->unlink) {
		errno = ENOSYS;
		goto cleanup_fromto;
	}

	errno = -(*inode_to->i_op->unlink)(inode_to, filename);

	if (errno != 0) {
		goto cleanup_fromto;
	}

	inode_from->link_count--;

	// Success
	(*inode_to->sb->s_op->write_inode)(inode_to);
	(*inode_from->sb->s_op->write_inode)(inode_from);
	errno = 0;

cleanup_fromto:
	(*inode_to->sb->s_op->free_inode)(inode_to);
cleanup_from:
	(*inode_from->sb->s_op->free_inode)(inode_from);
	return -errno;
}

int syscall_symlink(int path1p, int path2p, int c) {
	task_t *proc;
	pathname_t path;
	char *filename;
	inode_t *inode;
	int i;

	proc = task_list + task_current_pid();

	errno = task_access_memory(path1p);
	if (errno != 0) {
		return -errno;
	}
	errno = task_access_memory(path2p);
	if (errno != 0) {
		return -errno;
	}

	// Open destination file
	strcpy(path, proc->wd);
	errno = -path_cd(path, (char *)path2p);
	if (errno != 0) {
		return -errno;
	}
	// Find base filename
	filename = NULL;
	for (i = 1; path[i]; i++) {
		if (path[i] == '/') {
			filename = path + i;
		}
	}
	if (!filename) {
		return -EINVAL;
	}
	*filename = '\0';
	filename++;
	if (!*filename) {
		return -EINVAL;
	}
	if (!(inode = file_lookup(path))) {
		goto cleanup;
	}
	errno = -file_permission(inode, proc->uid, proc->gid, S_IW);
	if (errno != 0) {
		// Permission denied
		goto cleanup;
	}

	if (!inode->i_op->symlink) {
		errno = ENOSYS;
		goto cleanup;
	}

	errno = -(*inode->i_op->symlink)(inode, filename, (char *)path1p);

	// Success
	(*inode->sb->s_op->write_inode)(inode);
	errno = 0;

cleanup:
	(*inode->sb->s_op->free_inode)(inode);
	return -errno;
}

int syscall_readlink(int pathp, int bufp, int bufsize) {
	task_t *proc;
	pathname_t path;
	char *filename;
	inode_t *inode;
	int i, ret;

	errno = task_access_memory(pathp);
	if (errno != 0) {
		return -errno;
	}

	proc = task_list + task_current_pid();

	// Open destination file
	strcpy(path, proc->wd);
	errno = -path_cd(path, (char *)pathp);
	if (errno != 0) {
		return -errno;
	}
	// Find base filename
	filename = NULL;
	for (i = 1; path[i]; i++) {
		if (path[i] == '/') {
			filename = path + i;
		}
	}
	if (!filename) {
		return -EINVAL;
	}
	*filename = '\0';
	filename++;
	if (!*filename) {
		return -EINVAL;
	}
	if (!(inode = file_lookup(path))) {
		goto cleanup;
	}
	errno = -file_permission(inode, proc->uid, proc->gid, S_IW);
	if (errno != 0) {
		// Permission denied
		goto cleanup;
	}

	if (!inode->i_op->readlink) {
		errno = ENOSYS;
		goto cleanup;
	}

	ret = (*inode->i_op->readlink)(inode, path);
	if (ret < 0) {
		errno = -ret;
		goto cleanup;
	}
	if (ret >= bufsize) {

		errno = ERANGE;
		goto cleanup;
	}
	strcpy((char *)bufp, path);

	// Success
	errno = 0;

cleanup:
	(*inode->sb->s_op->free_inode)(inode);
	return -errno;
}

int syscall_truncate(int fd, int length, int c) {
	task_t *proc;
	file_t *file;
	int orig_length, ret;

	proc = task_list + task_current_pid();

	file = proc->files[fd];
	if (!file || fd >= TASK_MAX_OPEN_FILES || fd < 0) {
		return -EBADF;
	}
	if (!file->inode->i_op->truncate) {
		return -ENOSYS;
	}
	orig_length = file->inode->size;
	file->inode->size = length;
	ret = (*file->inode->i_op->truncate)(file->inode);
	if (ret < 0) {
		// Cancel truncate
		file->inode->size = orig_length;
	}
	return ret;
}

int syscall_rename(int oldpathp, int newpathp, int c) {
	int ret;
	ret = syscall_link(oldpathp, newpathp, 0);
	if (ret != 0) {
		return ret;
	}
	ret = syscall_unlink(oldpathp, 0, 0);
	if (ret != 0) {
		syscall_unlink(newpathp, 0, 0);
		return ret;
	}
	return 0;
}

int syscall_mkdir(int pathp, int mode, int c) {
	task_t *proc;
	pathname_t path;
	char *filename;
	inode_t *inode;
	int i;

	proc = task_list + task_current_pid();

	errno = task_access_memory(pathp);
	if (errno != 0) {
		return -errno;
	}

	// Open containing directory
	strcpy(path, proc->wd);
	errno = -path_cd(path, (char *)pathp);
	if (errno != 0) {
		return -errno;
	}
	// Find base filename
	filename = NULL;
	for (i = 1; path[i]; i++) {
		if (path[i] == '/') {
			filename = path + i;
		}
	}
	if (!filename) {
		return -EINVAL;
	}
	*filename = '\0';
	filename++;
	if (!*filename) {
		return -EINVAL;
	}
	if (!(inode = file_lookup(path))) {
		goto cleanup;
	}
	errno = -file_permission(inode, proc->uid, proc->gid, S_IW);
	if (errno != 0) {
		// Permission denied
		goto cleanup;
	}

	if (!inode->i_op->mkdir) {
		errno = ENOSYS;
		goto cleanup;
	}

	errno = -(*inode->i_op->mkdir)(inode, filename, mode);

	// Success
	(*inode->sb->s_op->write_inode)(inode);
	errno = 0;

cleanup:
	(*inode->sb->s_op->free_inode)(inode);
	return -errno;
}

int syscall_rmdir(int pathp, int b, int c) {
	task_t *proc;
	pathname_t path;
	char *filename;
	inode_t *inode;
	int i;

	proc = task_list + task_current_pid();

	errno = task_access_memory(pathp);
	if (errno != 0) {
		return -errno;
	}

	// Open containing directory
	strcpy(path, proc->wd);
	errno = -path_cd(path, (char *)pathp);
	if (errno != 0) {
		return -errno;
	}
	// Find base filename
	filename = NULL;
	for (i = 1; path[i]; i++) {
		if (path[i] == '/') {
			filename = path + i;
		}
	}
	if (!filename) {
		return -EINVAL;
	}
	*filename = '\0';
	filename++;
	if (!*filename) {
		return -EINVAL;
	}
	if (!(inode = file_lookup(path))) {
		goto cleanup;
	}
	errno = -file_permission(inode, proc->uid, proc->gid, S_IW);
	if (errno != 0) {
		// Permission denied
		goto cleanup;
	}

	if (!inode->i_op->rmdir) {
		errno = ENOSYS;
		goto cleanup;
	}

	errno = -(*inode->i_op->rmdir)(inode, filename);

	// Success
	(*inode->sb->s_op->write_inode)(inode);
	errno = 0;

cleanup:
	(*inode->sb->s_op->free_inode)(inode);
	return -errno;
}

int syscall_ioctl(int fd, int cmd, int arg) {
	task_t *proc;
	file_t *file;

	proc = task_list + task_current_pid();

	file = proc->files[fd];
	if (!file || fd >= TASK_MAX_OPEN_FILES || fd < 0) {
		return -EBADF;
	}
	if (!file->f_op->ioctl) {
		return -ENOSYS;
	}
	return (*file->f_op->ioctl)(file, cmd, arg);
}
