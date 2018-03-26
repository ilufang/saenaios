/**
 *	@file sys/mount.h
 *
 *	System calls to mount/umount filesystems
 */
#ifndef SYS_MOUNT_H
#define SYS_MOUNT_H

/**
 *	Mount a file system
 *
 *	@param source: the path to the device to mount
 *	@param target: the path to the directory to mount on
 *	@param filesystemtype: the type of the file system
 *	@param mountflags: flags
 *	@param opts: other options for the file system driver
 *	@return 0 on success, or negative of errno on failure
 */
int mount(const char *source, const char *target, const char *filesystemtype,
		  unsigned long mountflags, const char *opts);

/**
 *	Unmount a file system
 *
 *	@param target: the path to the mountpoint to be unmounted
 *	@return 0 on success, or negative of errno on failure
 */
int umount(const char *target);

#endif
