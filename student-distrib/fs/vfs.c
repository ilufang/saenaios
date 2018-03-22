#include "vfs.h"

#include "../errno.h"

#include "../proc/task.h"


int syscall_ece391_open(int pathaddr, int b, int c) {
	return syscall_open(pathaddr, 0, 0);
}

int syscall_open(int pathaddr, int flags, int mode) {
	task_t *proc;
	
	proc = task_list + task_current_pid();
	if (proc->status != TASK_ST_RUNNING) {
		return -ESRCH;
	}
	return 0;
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
