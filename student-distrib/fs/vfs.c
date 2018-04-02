#include "vfs.h"


#include "../lib.h"
#include "../errno.h"

#include "file_lookup.h"
#include "../proc/task.h"
#include "../proc/pcb.h"

#include "../../libc/src/syscalls.h" // Definitions from libc

file_t vfs_files[VFS_MAX_OPEN_FILES];

int syscall_ece391_open(int pathaddr, int b, int c) {
	return syscall_open(pathaddr, 0, 0);
}

int syscall_open(int pathaddr, int flags, int mode) {
	task_t *proc;
	pcb_t* curr_pcb = get_curr_pcb();
	pathname_t path = "/";
	int i, avail_fd = -1;
	inode_t *inode;
	file_t *file;

	proc = task_list + task_current_pid();
	if (proc->status != TASK_ST_RUNNING) {
		return -ESRCH;
	}

	errno = -path_cd(path, (char *)pathaddr);
	if (errno != 0) {
		return -errno;
	}

	for (i = 0; i < TASK_MAX_OPEN_FILES; i++) {
		/*
		if (proc->files[i] == NULL) {
			avail_fd = i;
			break;
		}*/
		if (curr_pcb->fd_arr[i] == NULL) {
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
	curr_pcb->fd_arr[avail_fd] = file;
	proc->files[avail_fd] = file;
	return avail_fd;
}

int syscall_close(int fd, int b, int c) {
	task_t *proc;
	file_t *file;

	pcb_t* curr_pcb = get_curr_pcb();
	proc = task_list + task_current_pid();
	if (proc->status != TASK_ST_RUNNING) {
		return -ESRCH;
	}
	//file = proc->files[fd];
	file = curr_pcb->fd_arr[fd];
	if (!file || fd >= TASK_MAX_OPEN_FILES || fd < 0) {
		return -EBADF;
	}
	(*(file->f_op->release))(file->inode, file);
	file->open_count--;
	if (file->open_count == 0) {
		vfs_close_file(file);
	}
	proc->files[fd] = NULL;
	curr_pcb->fd_arr[fd] = NULL;
	return 0;
}

int syscall_ece391_read(int fd, int bufaddr, int size) {
	int ret;
	struct dirent dent;
	ret = syscall_read(fd, bufaddr, size);
	if (ret == EISDIR) {
		if (size < VFS_FILENAME_LEN) {
			return -EINVAL;
		}
		ret = syscall_getdents(fd, (int)&dent, 0);
		if (ret == 0) {
			strcpy((char *)bufaddr, dent.filename);
		}
	}
	return ret;
}

int syscall_read(int fd, int bufaddr, int count) {
	
	task_t *proc;
	file_t *file;

	pcb_t* curr_pcb = get_curr_pcb();
	
	if (!bufaddr) {
		return -EINVAL;
	}

	proc = task_list + task_current_pid();
	if (proc->status != TASK_ST_RUNNING) {
		return -ESRCH;
	}
	//file = proc->files[fd];
	file = curr_pcb->fd_arr[fd];
	if (!file || fd >= TASK_MAX_OPEN_FILES || fd < 0) {
		return -EBADF;
	}
	if (!file->f_op->read) {
		return -ENOSYS;
	}
	// TODO: no permission check
	return (*file->f_op->read)(file, (uint8_t *) bufaddr, count, &(file->pos));
}

int syscall_write(int fd, int bufaddr, int count) {
	task_t *proc;
	file_t *file;
	pcb_t* curr_pcb = get_curr_pcb();
	
	if (!bufaddr) {
		return -EINVAL;
	}

	proc = task_list + task_current_pid();
	if (proc->status != TASK_ST_RUNNING) {
		return -ESRCH;
	}
	//file = proc->files[fd];
	file = curr_pcb->fd_arr[fd];
	if (!file || fd >= TASK_MAX_OPEN_FILES || fd < 0) {
		return -EBADF;
	}
	if (!file->f_op->write) {
		return -ENOSYS;
	}
	// TODO: no permission check
	return (*file->f_op->write)(file, (uint8_t *) bufaddr, count, &(file->pos));
}

int syscall_getdents(int fd, int bufaddr, int c) {
	task_t *proc;
	file_t *file;

	pcb_t* curr_pcb = get_curr_pcb;
	
	if (!bufaddr) {
		return -EINVAL;
	}

	proc = task_list + task_current_pid();
	if (proc->status != TASK_ST_RUNNING) {
		return -ESRCH;
	}
	//file = proc->files[fd];
	file = curr_pcb -> fd_arr[fd];
	if (!file || fd >= TASK_MAX_OPEN_FILES || fd < 0) {
		return -EBADF;
	}
	if (!file->f_op->readdir) {
		return -ENOSYS;
	}
	// TODO: no permission check
	return (*file->f_op->readdir)(file, (struct dirent *)bufaddr);
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

