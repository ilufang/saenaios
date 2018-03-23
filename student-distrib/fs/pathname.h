/**
 *	@file fs/pathname.h
 *
 *	Utilities to refactor absolute and relative path names
 */
#ifndef FS_PATHNAME_H
#define FS_PATHNAME_H

#define PATH_MAX_LEN	1024

typedef char pathname_t[PATH_MAX_LEN];

int path_cd(pathname_t path, const char *relpath);

#endif
