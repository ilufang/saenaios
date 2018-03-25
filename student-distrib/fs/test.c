#include "test.h"

#ifdef RUN_TESTS

#include "../tests.h"

#include "../lib.h"
#include "../libc.h"
#include "../errno.h"

#include "vfs.h"
#include "fstab.h"
#include "fs_devfs.h"
#include "pathname.h"

#include

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}

int fs_test_pathname() {
	TEST_HEADER;
	pathname_t base, rel;
	int ret = PASS;

	// Input sanity
	ret = path_cd(NULL, NULL);
	if (ret != -EINVAL) {
		printf("path_cd: null pointer not reported\n");
		ret = FAIL;
	}
	// Base address not absolute
	strcpy(base, "invalid/path");
	strcpy(rel, "a/b/c");
	ret = path_cd(base, rel);
	if (ret != -EINVAL) {
		printf("path_cd: non-absolute base address not enforced\n");
		ret = FAIL;
	}
	// Simple cd
	strcpy(base, "/1/2");
	ret = path_cd(base, rel);
	if (ret != 0) {
		printf("path_cd: regular cd failed\n");
		ret = FAIL;
	} else if (strncmp(base, "/1/2/a/b/c", PATH_MAX_LEN) != 0) {
		printf("path_cd: regular cd produced %s\n", base);
		ret = FAIL;
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
		ret = FAIL;
	}
	// Cd to absolute path
	strcpy(base, "/a/b/c");
	strcpy(rel, "/1/2/3");
	ret = path_cd(base, rel);
	if (ret != 0) {
		printf("path_cd: absolute cd failed\n");
		ret = FAIL;
	} else if (strncmp(base, "/1/2/3", PATH_MAX_LEN) != 0) {
		printf("path_cd: absolute cd produced %s\n", base);
		ret = FAIL;
	}
	// Cd with '.' and '..'
	strcpy(base, "/a/b/c/d/");
	strcpy(rel, "./../1/.//./_/./../2/3/");
	ret = path_cd(base, rel);
	if (ret != 0) {
		printf("path_cd: cd to parent failed\n");
		ret = FAIL;
	} else if (strncmp(base, "/a/b/c/1/2/3/", PATH_MAX_LEN) != 0) {
		printf("path_cd: cd to parent produced %s\n", base);
		ret = FAIL;
	}

	return ret;
}

void fs_test() {
	TEST_OUTPUT("VFS: path manipulation", fs_test_pathname);
}

#endif
