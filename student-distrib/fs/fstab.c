#include "fstab.h"

#include "../errno.h"
#include "../lib.c"

#include "../../libc/src/syscalls.h" // Definitions from libc

#define FSTAB_MAX_FS	16
#define FSTAB_MAX_MNT	16

file_system_t fstab_fs[FSTAB_MAX_FS];
vfsmount_t fstab_mnt[FSTAB_MAX_MNT];

int fstab_register_fs(file_system_t *fs) {
	int avail_idx = -1, i;

	if (!fs || fs->name[0] == '\0') {
		return -EINVAL;
	}

	for (i = 0; i < FSTAB_MAX_FS; i++) {
		if (avail_idx < 0 && fstab_fs[i].name[0] == '\0') {
			avail_idx = i;
		} else if (strncmp(fstab_fs[i].name, fs->name, FSTAB_FS_NAME_LEN)==0) {
			return -EEXIST;
		}
	}

	if (avail_idx < 0) {
		return -ENFILE;
	}

	memcpy(fstab_fs + avail_idx, fs, sizeof(file_system_t));

	return 0;
}

int fstab_unregister_fs(const char *name) {
	int i;

	if (!name) {
		return -EINVAL;
	}

	for (i = 0; i < FSTAB_MAX_FS; i++) {
		if (strncmp(fstab_fs[i].name, name, FSTAB_FS_NAME_LEN)==0) {
			fstab_fs[i].name[0] = '\0'; // Mark as deleted
			return 0;
		}
	}

	return -ENOENT;
}

file_system_t *fstab_get_fs(const char *name) {
	int i;

	if (!name) {
		errno = EINVAL;
		return NULL;
	}

	for (i = 0; i < FSTAB_MAX_FS; i++) {
		if (strncmp(fstab_fs[i].name, name, FSTAB_FS_NAME_LEN)==0) {
			return fstab_fs + i;
		}
	}

	errno = ENOENT;
	return NULL;
}

vfsmount_t *fstab_get_mountpoint(const char *path, int *offset) {
	int max_match = 0, max_idx = -1, i, j;
	char *mntname;

	if (!path || !offset || path[0] != '/') {
		errno = EINVAL;
		return NULL;
	}

	for (i = 0; i < FSTAB_MAX_MNT; i++) {
		mntname = fstab_mnt[i].mountpoint;
		for (j = 0; path[j] == mntname[j] && path[j] != '\0'; j++);
		if (j > max_match) {
			max_match = j;
			max_idx = i;
		}
	}

	if (max_idx < 0) {
		errno = ENOENT;
		return NULL;
	}

	*offset = max_match;
	return fstab_mnt[max_idx];
}

int syscall_mount(int typeaddr, int destaddr, int optaddr) {
	char *type;
	path_t dest = "/";
	struct sys_mount_opts *opts;
	int i, avail_idx = -1;
	file_system_t *fs = NULL;

	if (!typeaddr || !destaddr || !optaddr) {
		return -EINVAL;
	}
	type = (char *) typeaddr;
	opts = (struct sys_mount_opts *) optaddr;
	errno = -path_cd(dest, (char *)destaddr);
	if (errno != 0) {
		return -errno;
	}

	// TODO: permission checks

	// Lookup filesystem by name
	for (i = 0; i < FSTAB_MAX_FS; i++) {
		if (strncmp(type, fstab_fs[i].name, FSTAB_FS_NAME_LEN) == 0) {
			fs = fstab_fs + i;
			break;
		}
	}

	if (!fs) {
		return -ENODEV;
	}

	for (i = 0; i < FSTAB_MAX_MNT; i++) {
		if (avail_idx < 0 && fstab_mnt[i].mountpoint[0] == '\0') {
			avail_idx = i;
		} else if (strncmp(dest, fstab_mnt[i].mountpoint, PATH_MAX_LEN) == 0) {
			return -EEXIST;
		}
	}

	if (!avail_idx) {
		return -ENFILE;
	}

	strcpy(dest, fstab_mnt[avail_idx].mountpoint);

	fstab_mnt[avail_idx].superblock = (*fs->get_sb)(fs, opts->mountflags,
													opts->source, opts->opts);
	fstab_mnt[avail_idx].count = 0;
	fstab_mnt[avail_idx].root = fstab_mnt[avail_idx].superblock->root;

	return 0;
}

int syscall_umount(int targetaddr, int b, int c) {
	path_t target = "/";
	int i;

	if (!targetaddr) {
		return -EINVAL;
	}

	errno = -path_cd(target, (char *)targetaddr);
	if (errno != 0) {
		return -errno;
	}

	// TODO: permission checks

	for (i = 0; i < FSTAB_MAX_MNT; i++) {
		if (strncmp(target, fstab_mnt[i].mountpoint) == 0) {
			if (fstab_mnt[i].count != 0) {
				// There are open files
				return -EBUSY;
			}
			fstab_mnt[i].fs->kill_sb(fstab_mnt[i].superblock);
			fstab_mnt[i].mountpoint[0] = '\0';
			return 0;
		}
	}
	return -ENOENT;
}
