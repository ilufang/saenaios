/**
 *	@file sys/mount.h
 *
 *	System calls to mount/umount filesystems
 */
#ifndef SYS_MOUNT_H
#define SYS_MOUNT_H

int mount(const char *source, const char *target, const char *filesystemtype,
		  unsigned long mountflags, const char *opts);

int umount(const char *target);

#endif
