#include "syscalls.h"

#include "../include/stddef.h"
#include "../include/sys/types.h"
#include "../include/sys/stat.h"
#include "../include/errno.h"
#include "../include/unistd.h"
#include "../include/signal.h"
#include "../include/sys/wait.h"
#include "../include/sys/mount.h"

int do_syscall(int num, int b, int c, int d);

int mount(const char *source, const char *target, const char *filesystemtype,
		  unsigned long mountflags, const char *opts) {
	int ret;
	struct sys_mount_opts mount_opts;
	mount_opts.source = source;
	mount_opts.mountflags = mountflags;
	mount_opts.opts = opts;
	ret = do_syscall(SYSCALL_MOUNT, (int)filesystemtype, (int)target,
					 (int)(&mount_opts));
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int umount(const char *target) {
	int ret;
	ret = do_syscall(SYSCALL_UMOUNT, (int)target, 0, 0);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int open(const char *pathname, int flags, int mode) {
	int ret;
	ret = do_syscall(SYSCALL_OPEN, (int)pathname, flags, mode);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int close(int fd) {
	int ret;
	ret = do_syscall(SYSCALL_CLOSE, fd, 0, 0);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

ssize_t read(int fd, void *buf, size_t count) {
	int ret;
	ret = do_syscall(SYSCALL_READ, fd, (int)buf, count);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

ssize_t write(int fd, const void *buf, size_t count) {
	int ret;
	ret = do_syscall(SYSCALL_WRITE, fd, (int)buf, count);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int getdents(int fd, struct dirent *buf) {
	int ret;
	ret = do_syscall(SYSCALL_GETDENTS, fd, (int)buf, 0);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

pid_t fork() {
	int ret;
	ret = do_syscall(SYSCALL_FORK, 0, 0, 0);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int stat(const char* path, struct s_stat* buf){
	int ret;
	ret = do_syscall(SYSCALL_STAT, (int)path, (int)buf, 0);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int fstat(int fd, struct s_stat* buf){
	int ret;
	ret = do_syscall(SYSCALL_FSTAT, fd, (int)buf, 0);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int lstat(const char* path, struct s_stat* buf){
	int ret;
	ret = do_syscall(SYSCALL_LSTAT, (int)path, (int)buf, 0);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int execve(const char *path, char *const argv[], char *const envp[]) {
	int ret;
	ret = do_syscall(SYSCALL_EXECVE, (int)path, (int)argv, (int)envp);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

void _exit(int status) {
	do_syscall(SYSCALL__EXIT, (uint8_t)status, 0, 0);
}

pid_t wait(int *status) {
	return do_syscall(SYSCALL_WAITPID, -1, (int)status, 0);
}

pid_t waitpid(pid_t pid, int *status, int options) {
	int ret;
	ret = do_syscall(SYSCALL_WAITPID, (int)pid, (int)status, options);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

sig_t signal(int sig, sig_t handler) {
	int ret;
	struct sigaction act, oldact;
	sigemptyset(&act.mask);
	act.flags = SA_RESTART;
	ret = sigaction(sig, &act, &oldact);
	if (ret < 0) {
		return SIG_ERR;
	}
	return oldact.handler;
}

int sigaction(int sig, const struct sigaction *act, struct sigaction *oldact) {
	int ret;
	ret = do_syscall(SYSCALL_SIGACTION, sig, (int) act, (int) oldact);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int sigprocmask(int how, sigset_t *set, sigset_t *oldset) {
	int ret;
	ret = do_syscall(SYSCALL_SIGPROCMASK, how, (int) set, (int) oldset);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int sigsuspend(sigset_t *sigset) {
	int ret;
	ret = do_syscall(SYSCALL_SIGSUSPEND, (int) sigset, 0, 0);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int kill(pid_t pid, int sig) {
	int ret;
	ret = do_syscall(SYSCALL_KILL, pid, sig, 0);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int brk(void* addr) {
	return do_syscall(SYSCALL_BRK, (int)addr, 0, 0);
}

void* sbrk(int increment) {
	return (void*)do_syscall(SYSCALL_SBRK, increment, 0, 0);
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

off_t lseek(int fd, off_t offset, int whence) {
	int ret;
	ret = do_syscall(SYSCALL_LSEEK, fd, offset, whence);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

pid_t getpid() {
	int ret;
	ret = do_syscall(SYSCALL_GETPID, 0, 0, 0);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int chmod(const char *path, mode_t mode) {
	int fd, ret;
	fd = open(path, O_RDONLY, 0);
	if (!fd)
		return fd;
	ret = fchmod(fd, mode);
	close(fd);
	return ret;
}

int fchmod(int fd, mode_t mode) {
	int ret;
	ret = do_syscall(SYSCALL_CHMOD, fd, mode, 0);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int chown(const char *path, uid_t uid, gid_t gid) {
	int fd, ret;
	fd = open(path, O_RDONLY, 0);
	if (!fd)
		return fd;
	ret = fchown(fd, uid, gid);
	close(fd);
	return ret;
}

int fchown(int fd, uid_t uid, gid_t gid) {
	int ret;
	ret = do_syscall(SYSCALL_CHOWN, fd, uid, gid);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int link(const char *path1, const char *path2) {
	int ret;
	ret = do_syscall(SYSCALL_LINK, (int) path1, (int) path2, 0);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int unlink(const char *path) {
	int ret;
	ret = do_syscall(SYSCALL_UNLINK, (int) path, 0, 0);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int symlink(const char *path1, const char *path2) {
	int ret;
	ret = do_syscall(SYSCALL_SYMLINK, (int) path1, (int) path2, 0);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

ssize_t readlink(const char *path, char *buf, size_t bufsize) {
	int ret;
	ret = do_syscall(SYSCALL_READLINK, (int) path, (int) buf, bufsize);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int truncate(const char *path, off_t length) {
	int fd, ret;
	fd = open(path, O_RDONLY, 0);
	if (!fd)
		return fd;
	ret = ftruncate(fd, length);
	close(fd);
	return ret;
}

int ftruncate(int fd, off_t length) {
	int ret;
	ret = do_syscall(SYSCALL_TRUNCATE, fd, length, 0);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int rename(const char *oldpath, const char *newpath) {
	int ret;
	ret = do_syscall(SYSCALL_RENAME, (int) oldpath, (int) newpath, 0);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

char *getcwd(char *buf, size_t size) {
	int ret;
	ret = do_syscall(SYSCALL_GETCWD, (int) buf, size, 0);
	if (ret < 0) {
		errno = -ret;
		return NULL;
	}
	return buf;
}


int chdir(const char *path) {
	int ret;
	ret = do_syscall(SYSCALL_CHDIR, (int) path, 0, 0);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int mkdir(const char *path, mode_t mode) {
	int ret;
	ret = do_syscall(SYSCALL_MKDIR, (int) path, mode, 0);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int rmdir(const char *path) {
	int ret;
	ret = do_syscall(SYSCALL_MKDIR, (int) path, 0, 0);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}

int ioctl(int fd, int cmd, int arg) {
	int ret;
	ret = do_syscall(SYSCALL_IOCTL, fd, cmd, arg);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return ret;
}
