#include "syscalls.h"

#include "../include/stddef.h"
#include "../include/sys/types.h"
#include "../include/sys/stat.h"
#include "../include/errno.h"
#include "../include/signal.h"

#include "../include/sys/mount.h"

int do_syscall(int num, int b, int c, int d);

int mount(const char *source, const char *target, const char *filesystemtype,
		  unsigned long mountflags, const char *opts) {
	struct sys_mount_opts mount_opts;
	mount_opts.source = source;
	mount_opts.mountflags = mountflags;
	mount_opts.opts = opts;
	return do_syscall(SYSCALL_MOUNT, (int)filesystemtype, (int)target,
					  (int)(&mount_opts));
}

int umount(const char *target) {
	return do_syscall(SYSCALL_UMOUNT, (int)target, 0, 0);
}

int open(const char *pathname, int flags, int mode) {
	return do_syscall(SYSCALL_OPEN, (int)pathname, flags, mode);
}

int close(int fd) {
	return do_syscall(SYSCALL_CLOSE, fd, 0, 0);
}

int read(int fd, void *buf, unsigned int count) {
	return do_syscall(SYSCALL_READ, fd, (int)buf, count);
}

int write(int fd, const void *buf, unsigned int count) {
	return do_syscall(SYSCALL_WRITE, fd, (int)buf, count);
}

int getdents(int fd, struct dirent *buf) {
	return do_syscall(SYSCALL_GETDENTS, fd, (int)buf, 0);
}

pid_t fork() {
	return do_syscall(SYSCALL_FORK, 0, 0, 0);
}

int stat(const char* path, struct s_stat* buf){
	return do_syscall(SYSCALL_STAT, (int)path, (int)buf, 0);
}

int fstat(int fd, struct s_stat* buf){
	return do_syscall(SYSCALL_FSTAT, fd, (int)buf, 0);
}

int lstat(const char* path, struct s_stat* buf){
	return do_syscall(SYSCALL_LSTAT, (int)path, (int)buf, 0);
}

int execve(const char *path, char *const argv[], char *const envp[]) {
	return do_syscall(SYSCALL_EXECVE, (int)path, (int)argv, (int)envp);
}

void _exit(int status) {
	do_syscall(SYSCALL__EXIT, status, 0, 0);
}

pid_t wait(int *status) {
	return do_syscall(SYSCALL_WAIT, (int)status, 0, 0);
}

int sigaction(int sig, const struct sigaction *act, struct sigaction *oldact) {
	return do_syscall(SYSCALL_SIGACTION, sig, (int) act, (int) oldact);
}

int sigprocmask(int how, sigset_t *set, sigset_t *oldset) {
	return do_syscall(SYSCALL_SIGPROCMASK, how, (int) set, (int) oldset);
}

int sigsuspend(sigset_t *sigset) {
	return do_syscall(SYSCALL_SIGSUSPEND, (int) sigset, 0, 0);
}

int kill(pid_t pid, int sig) {
	return do_syscall(SYSCALL_KILL, pid, sig, 0);
}

#define LIBC_MAX_OPEN_DIR	64

static DIR libc_dir_list[LIBC_MAX_OPEN_DIR];

DIR *opendir(const char *filename) {
	int fd;
	fd = open(filename, O_RDONLY, 0);
	if (fd >= 0) {
		return fdopendir(fd);
	}
	return NULL;
}

DIR *fdopendir(int fd) {
	int i;
	for (i = 0; i < LIBC_MAX_OPEN_DIR; i++) {
		if (libc_dir_list[i].count == 0) {
			break;
		}
	}
	if (i == LIBC_MAX_OPEN_DIR) {
		errno = ENFILE;
		return NULL;
	}
	libc_dir_list[i].count = 1;
	libc_dir_list[i].fd = fd;
	libc_dir_list[i].dent.index = -1;
	return libc_dir_list + i;
}

struct dirent *readdir(DIR *dirp) {
	if (!dirp || dirp->count == 0) {
		errno = EINVAL;
		return NULL;
	}
	errno = -getdents(dirp->fd, &(dirp->dent));
	if (errno == 0)
		return &(dirp->dent);
	else
		return NULL;
}

long telldir(DIR *dirp) {
	if (!dirp || dirp->count == 0) {
		return -EINVAL;
	}
	return dirp->dent.index;
}

void seekdir(DIR *dirp, long loc) {
	if (!dirp || dirp->count == 0) {
		errno = EINVAL;
		return;
	}
	dirp->dent.index = loc;
}

void rewinddir(DIR *dirp) {
	if (!dirp || dirp->count == 0) {
		errno = EINVAL;
		return;
	}
	dirp->dent.index = -1;
}

int closedir(DIR *dirp) {
	if (!dirp || dirp->count == 0) {
		return -EINVAL;
	}
	dirp->count = 0;
	return close(dirp->fd);
}

int dirfd(DIR *dirp) {
	if (!dirp || dirp->count == 0) {
		return -EINVAL;
	}
	return dirp->fd;

}
