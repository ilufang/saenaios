#include "vfs.h"

#include "../errno.h"

#include "../proc/task.h"


int syscall_ece391_open(int pathaddr, int b, int c) {
	return syscall_open(pathaddr, 0, 0);
}

int syscall_open(int pathaddr, int flags, int mode) {
	task_t *proc;
	pathname_t path = "/";
	int i, avail_fd = -1;
	vfsmount_t *fs;
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
		if (proc->files[i] == NULL) {
			avail_fd = i;
			break;
		}
	}
	if (avail_fd < 0) {
		return -EMFILE;
	}

/*	if (!(fs = fstab_get_mountpoint(path, &i))) {
		return -errno;
	}

	if (!(inode = find_inode(fs, path + i))) { // TODO
		return -errno;
	}

	if (!(file = alloc_file())) {; // TODO
		return -errno;
	}*/
	if (!(inode = file_lookup(path))){
		return -errno;
	}

	errno = -(*file->f_op->open)(inode, file);
	if (errno != 0) {
		dealloc_file(); // TODO
		return -errno;
	}

	proc->file[avail_fd] = file;
	return avail_fd;
}

int syscall_close(int fd, int b, int c) {
	task_t *proc;
	file_t *file;

	proc = task_list + task_current_pid();
	if (proc->status != TASK_ST_RUNNING) {
		return -ESRCH;
	}
	file = proc->files[fd];
	if (!file || fd >= 16 || fd < 0) {
		return -EBADF;
	}
	(*(file->f_op->release))(file->inode, file);
	file->open_count--;
	if (file->open_count == 0) {
		//vfs_close_file(inode, file);
	}
	proc->files[fd] = NULL;
	return 0;
}

int syscall_read(int fd, int bufaddr, int count) {
	task_t *proc;
	file_t *file;

	if (!bufaddr) {
		return -EINVAL;
	}

	proc = task_list + task_current_pid();
	if (proc->status != TASK_ST_RUNNING) {
		return -ESRCH;
	}
	file = proc->files[fd];
	if (!file || fd >= 16 || fd < 0) {
		return -EBADF;
	}
	// TODO: no permission check
	return (*file->f_op->read)(file, (uint8_t *) bufaddr, count, &(file->pos));
}

int syscall_write(int fd, int bufaddr, int count) {
	task_t *proc;
	file_t *file;

	if (!bufaddr) {
		return -EINVAL;
	}

	proc = task_list + task_current_pid();
	if (proc->status != TASK_ST_RUNNING) {
		return -ESRCH;
	}
	file = proc->files[fd];
	if (!file || fd >= 16 || fd < 0) {
		return -EBADF;
	}
	// TODO: no permission check
	return (*file->f_op->write)(file, (uint8_t *) bufaddr, count, &(file->pos));
}
