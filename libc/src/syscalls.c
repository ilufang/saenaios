#include "syscalls.h"

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
