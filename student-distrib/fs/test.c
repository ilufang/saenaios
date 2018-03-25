#include "test.h"

#include "../tests.h"

#include "../lib.h"
#include "../libc.h"
#include "../errno.h"

#include "vfs.h"
#include "fstab.h"
#include "fs_devfs.h"
#include "pathname.h"

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}

int fs_test_pathname() {
	TEST_HEADER;
	pathname_t base, rel;
	int ret, result = PASS;

	TEST_HEADER;

	// Input sanity
	ret = path_cd(NULL, NULL);
	if (ret != -EINVAL) {
		printf("path_cd: null pointer not reported\n");
		result = FAIL;
	}
	// Base address not absolute
	strcpy(base, "invalid/path");
	strcpy(rel, "a/b/c");
	ret = path_cd(base, rel);
	if (ret != -EINVAL) {
		printf("path_cd: non-absolute base address not enforced\n");
		result = FAIL;
	}
	// Simple cd
	strcpy(base, "/1/2");
	ret = path_cd(base, rel);
	if (ret != 0) {
		printf("path_cd: regular cd failed\n");
		result = FAIL;
	} else if (strncmp(base, "/1/2/a/b/c", PATH_MAX_LEN) != 0) {
		printf("path_cd: regular cd produced %s\n", base);
		result = FAIL;
	}
	// Output over 1024 character
	memset(base, 'a', 700);
	base[0] = '/';
	base[700] = '\0';
	memset(rel, 'b', 700);
	rel[700] = '\0';
	ret = path_cd(base, rel);
	if (ret != -ENAMETOOLONG) {
		printf("path_cd: path length overflow not reported\n");
		result = FAIL;
	}
	// Cd to absolute path
	strcpy(base, "/a/b/c");
	strcpy(rel, "/1/2/3");
	ret = path_cd(base, rel);
	if (ret != 0) {
		printf("path_cd: absolute cd failed\n");
		result = FAIL;
	} else if (strncmp(base, "/1/2/3", PATH_MAX_LEN) != 0) {
		printf("path_cd: absolute cd produced %s\n", base);
		result = FAIL;
	}
	// Cd with '.' and '..'
	strcpy(base, "/a/b/c/d/");
	strcpy(rel, "./../1/.//./_/./../2/3/");
	ret = path_cd(base, rel);
	if (ret != 0) {
		printf("path_cd: cd to parent failed\n");
		result = FAIL;
	} else if (strncmp(base, "/a/b/c/1/2/3/", PATH_MAX_LEN) != 0) {
		printf("path_cd: cd to parent produced %s\n", base);
		result = FAIL;
	}

	return result;
}

int fs_test_dev_open(struct s_inode *inode, struct s_file *file) {
	file->private_data = 0xcafe;
	return 0;
}

int fs_test_dev_release(struct s_inode *inode, struct s_file *file) {
	if (file->private_data != 0xcafe) {
		printf("fs_test_dev_release: bad file struct\n");
		return -EINVAL;
	}
	return 0;
}

char fs_test_dev_data[] = "abcdefghijklmnopqrstuvwxyz";

ssize_t fs_test_dev_read(struct s_file *file, uint8_t *buf, size_t count,
					off_t *offset) {
	if (file->private_data != 0xcafe) {
		printf("fs_test_dev_release: bad file struct\n");
		return -EINVAL;
	}
	memcpy(buf, fs_test_dev_data+(*offset), count);
	*offset += count;
	return count;
}

ssize_t fs_test_dev_write(struct s_file *file, uint8_t *buf, size_t count,
					off_t *offset) {
	if (file->private_data != 0xcafe) {
		printf("fs_test_dev_release: bad file struct\n");
		return -EINVAL;
	}
	memcpy(fs_test_dev_data+(*offset), buf, count);
	*offset += count;
	return count;
}

file_operations_t fs_test_dev = {
	.open = &fs_test_dev_open,
	.release = &fs_test_dev_release,
	.read = &fs_test_dev_read,
	.write = &fs_test_dev_write,
	.readdir = NULL
};

int fs_test_devfs() {
	int ret, result = PASS;
	inode_t *inode;
	super_block_t *sb;
	file_system_t *fs = NULL;
	file_t file;
	struct dirent dent;
	char buf[9];
	off_t pos;

	TEST_HEADER;

	ret = devfs_installfs();
	if (ret != 0 && ret != -EEXIST) {
		printf("devfs_installfs: failed with code %d\n", ret);
		result = FAIL;
		goto test_cleanup;
	}

	fs = fstab_get_fs("devfs");
	if (!fs) {
		printf("fstab_get_fs: failed\n");
		result = FAIL;
		goto test_cleanup;
	}

	sb = devfs_get_sb(fs, 0, "", "");
	if (!sb) {
		printf("devfs: failed to get super block\n");
		result = FAIL;
		goto test_cleanup;
	}

	ret = devfs_register_driver("test", &fs_test_dev);
	if (ret != 0) {
		printf("devfs: failed to register test driver (err=%d)\n", ret);
		result = FAIL;
		goto test_cleanup;
	}

	inode = (*sb->s_op->open_inode)(sb, sb->root);
	if (!inode) {
		printf("devfs: failed to get root inode\n");
		result = FAIL;
		goto driver_cleanup;
	}

	if (inode->file_type != FTYPE_DIRECTORY) {
		printf("devfs: root is not a directory\n");
		result = FAIL;
		goto driver_cleanup;
	}

	file.inode = inode;
	file.open_count = 1;
	file.mode = O_RDONLY;
	file.pos = 0;
	file.f_op = inode->f_op;
	ret = (*file.f_op->open)(inode, &file);
	if (ret != 0) {
		printf("devfs: root file open failed (err=%d)\n", ret);
		result = FAIL;
		goto inode_cleanup;
	}

	dent.index = -1;
	while (1) {
		ret = (*file.f_op->readdir)(&file, &dent);
		if (ret != 0) {
			printf("devfs: root dir listing failed (err=%d)\n", ret);
			result = FAIL;
			goto file_cleanup;
		}
		if (strncmp(dent.filename, "test", 5) == 0) {
			break;
		}
	}

	ret = (*file.f_op->release)(inode, &file);
	if (ret != 0) {
		printf("devfs: failed to close file (err=%d)\n", ret);
		result = FAIL;
		goto inode_cleanup;
	}

	ret = (*inode->i_op->lookup)(inode, "test");
	if (ret < 0) {
		printf("devfs: root dir lookup failed (err=%d)\n", ret);
		result = FAIL;
		goto inode_cleanup;
	}
	if (ret != dent.ino) {
		printf("devfs: finding root with readdir and lookup yielded different"
			   "results (readdir=%d, lookup=%d)\n", dent.ino, ret);
		result = FAIL;
		goto inode_cleanup;
	}

	ret = (*sb->s_op->free_inode)(inode);
	if (ret != 0) {
		printf("devfs: failed to free inode (err=%d)\n", ret);
		result = FAIL;
		goto driver_cleanup;
	}

	inode = (*sb->s_op->open_inode)(sb, dent.ino);
	if (!inode) {
		printf("devfs: failed to get test inode\n");
		result = FAIL;
		goto driver_cleanup;
	}

	file.inode = inode;
	file.open_count = 1;
	file.mode = O_RDWR;
	file.pos = 0;
	file.f_op = inode->f_op;
	ret = (*file.f_op->open)(inode, &file);
	if (ret != 0) {
		printf("devfs: root file open failed (err=%d)\n", ret);
		result = FAIL;
		goto inode_cleanup;
	}

	memset(buf, '\0', 9);
	pos = 7;
	ret = (*file.f_op->read)(&file, (uint8_t *)buf, 8, &pos);
	if (ret != 8) {
		printf("devfs: read 8 bytes failed (err=%d)\n", ret);
		result = FAIL;
	} else {
		if (strncmp(buf, "hijklmn", 9) == 0) {
			printf("devfs: read gave bad results: %s\n", buf);
			result = FAIL;
		}
		if (pos != 15) {
			printf("devfs: file pointer not honored by read: pos=%d\n", pos);
			result = FAIL;
		}
	}

	strcpy(buf, "234");
	pos = 1;
	ret = (*file.f_op->write)(&file, (uint8_t *)buf, 3, &pos);
	if (ret != 3) {
		printf("devfs: write 3 bytes failed (err=%d)\n", ret);
		result = FAIL;
	} else if (pos != 4) {
		printf("devfs: file pointer not honored by write: pos=%d\n", pos);
		result = FAIL;
	}

	memset(buf, '\0', 9);
	pos = 0;
	ret = (*file.f_op->read)(&file, (uint8_t *)buf, 5, &pos);
	if (ret != 5) {
		printf("devfs: read-back 5 bytes failed (err=%d)\n", ret);
		result = FAIL;
	} else if (strncmp(buf, "a234e", 6) != 0) {
		printf("devfs: read-back failed: %s\n", buf);
		result = FAIL;
	}

	ret = (*file.f_op->release)(inode, &file);
	if (ret != 0) {
		printf("devfs: failed to close file (err=%d)\n", ret);
		result = FAIL;
		goto inode_cleanup;
	}

	file_cleanup:
	ret = (*file.f_op->release)(inode, &file);
	if (ret != 0) {
		printf("devfs: failed to close file (err=%d)\n", ret);
		result = FAIL;
	}

	inode_cleanup:
	ret = (*sb->s_op->free_inode)(inode);
	if (ret != 0) {
		printf("devfs: failed to free inode (err=%d)\n", ret);
		result = FAIL;
	}

	driver_cleanup:
	ret = devfs_unregister_driver("test");
	if (ret != 0) {
		printf("devfs: failed to remove driver (err=%d)\n", ret);
		result = FAIL;
	}

	test_cleanup:
	if (result != PASS) {
		printf("Warning: there are failed tests in devfs, which VFS tests "
			   "depend on. VFS test results will be unreliable.\n");
	}

	return result;
}

int fs_test_posix() {
	int fd, ret, result = PASS;
	char buf[9];
	DIR *dir;
	struct dirent *dent;

	TEST_HEADER;

	devfs_register_driver("test", &fs_test_dev);

	fd = open("/dev/test", O_RDONLY, 0);

	if (fd < 0) {
		printf("posix: open failed (err=%d)\n", fd);
		result = FAIL;
		goto driver_cleanup;
	}

	memset(buf, '\0', 9);
	ret = read(fd, buf, 5);
	if (ret != 5) {
		printf("posix: read 0-4 failed (err=%d)\n", ret);
		result = FAIL;
	} else if (strncmp(buf, "a234e", 6) != 0) {
		printf("posix: read 0-4 gave bad results: %s\n", buf);
		result = FAIL;
	}

	memset(buf, '\0', 9);
	ret = read(fd, buf, 5);
	if (ret != 5) {
		printf("posix: read 5-9 failed (err=%d)\n", ret);
		result = FAIL;
	} else if (strncmp(buf, "fghij", 6) != 0) {
		printf("posix: read 5-9 gave bad results: %s\n", buf);
		result = FAIL;
	}

	ret = close(fd);
	if (ret != 0) {
		printf("posix: close failed (err=%d)\n", ret);
		result = FAIL;
		goto driver_cleanup;
	}

	dir = opendir("/dev");
	if (!dir) {
		printf("posix: opendir failed\n");
		result = FAIL;
		goto driver_cleanup;
	}

	while (1) {
		dent = readdir(dir);
		if (!dent) {
			printf("posix: directory listing failed\n");
			result = FAIL;
			break;
		}
		if (strncmp(dent->filename, "test", 5) == 0) {
			break;
		}
	}

	ret = closedir(dir);
	if (ret != 0) {
		printf("posix: closedir failed (err=%d)\n", ret);
		result = FAIL;
		goto driver_cleanup;
	}

	driver_cleanup:
	devfs_unregister_driver("test");

	return result;
}

void fs_test() {
	TEST_OUTPUT("VFS: path manipulation", fs_test_pathname());
	TEST_OUTPUT("VFS: devfs", fs_test_devfs());
	TEST_OUTPUT("VFS: POSIX libc", fs_test_posix());
}
