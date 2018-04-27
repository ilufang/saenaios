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
	ret = syscall_open(pathaddr, O_RDONLY, 0);
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
	pathname_t path = "/";
	int i, avail_fd = -1;
	inode_t *inode;
	file_t *file;

	proc = task_list + task_current_pid();

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
		return -errno;
	}

	if (!(file = vfs_open_file(inode, flags))) {
		return -errno;
	}

	errno = -(*file->f_op->open)(inode, file);
	if (errno != 0) {
		vfs_close_file(file);
		return -errno;
	}
	proc->files[avail_fd] = file;
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
		return -EINVAL;
	}

	proc = task_list + task_current_pid();

	file = proc->files[fd];
	if (!file || fd >= TASK_MAX_OPEN_FILES || fd < 0) {
		return -EBADF;
	}
	if (!file->f_op->read) {
		return -ENOSYS;
	}
	// TODO: no permission check
	return (*file->f_op->read)(file, (uint8_t *) bufaddr, count, &(file->pos));
}

int syscall_ece391_write(int fd, int bufaddr, int count) {
	int ret;
	ret = syscall_write(fd, bufaddr, count);
	if (ret < 0)
		return -1;
	return ret;
}

int syscall_write(int fd, int bufaddr, int count) {
	task_t *proc;
	file_t *file;

	if (!bufaddr) {
		return -EINVAL;
	}

	proc = task_list + task_current_pid();

	file = proc->files[fd];
	if (!file || fd >= TASK_MAX_OPEN_FILES || fd < 0) {
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
		return -EINVAL;
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
	// TODO: no permission check
	dent = (struct dirent *)bufaddr;
	if (dent->index == DIRENT_INDEX_AUTO) {
		// Workaround ece391_read auto dir listing
		dent->index = file->pos - 1;
		ret = (*file->f_op->readdir)(file, dent);
		if (ret >= 0) {
			file->pos++;
		}
		return ret;
	} else
		return (*file->f_op->readdir)(file, dent);
}

file_t *vfs_open_file(inode_t *inode, int mode) {
	int i, ret;

	if (!inode || !(mode & (O_RDONLY | O_WRONLY))) {
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
		return -EINVAL;
	}
	file->open_count--;
	if (file->open_count != 0) {
		// Someone else is still using this file
		return 0;
	}
	// Close this file
	(*file->f_op->release)(file->inode, file);
	(*file->inode->sb->s_op->free_inode)(file->inode);
	file->inode = NULL;
	file->mode = 0;
	file->f_op = NULL;
	return 0;
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
	//TODO
	return -ENOSYS;
}
