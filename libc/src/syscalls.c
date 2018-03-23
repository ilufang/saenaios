#include <sys/mount.h>

int do_syscall(int num, int b, int c, int d);

int mount(const char *source, const char *target, const char *filesystemtype,
		  unsigned long mountflags, const char *opts) {
	struct sys_mount_opts opts;
	opts.source = source;
	opts.mountflags = mountflags;
	opts.opts = data;
	return do_syscall(SYSCALL_MOUNT, (int)filesystemtype, (int)target,
					  (int)(&opts));
}

int umount(const char *target) {
	return do_syscall(SYSCALL_UMOUNT, (int)target);
}

int open(const char *pathname, int flags, int mode) {
	return do_syscall(SYSCALL_OPEN, (int)pathname, flags, mode);
}

int close(int fd) {
	return do_syscall(SYSCALL_CLOSE, fd, 0, 0);
}

ssize_t read(int fd, void *buf, size_t count) {
	return do_syscall(SYSCALL_READ, fd, (int)buf, count);
}

ssize_t write(int fd, const void *buf, size_t count) {
	return do_syscall(SYSCALL_WRITE, fd, (int)buf, count);
}
