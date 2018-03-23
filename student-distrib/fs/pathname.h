/**
 *	@file fs/pathname.h
 *
 *	Utilities to refactor absolute and relative path names
 */
#ifndef FS_PATHNAME_H
#define FS_PATHNAME_H

#define PATH_MAX_LEN	1024	///< Maximum length of a path

/**
 *	Pathname type.
 *
 *	pathname_t is defined as a char array. I.e, it will allocate stack space
 *	when directly declared, but will not make copies when used in function
 *	parameters.
 */
typedef char pathname_t[PATH_MAX_LEN];

/**
 *	Apply a relative path to an absolute path
 *
 *	This function will also refactor special components like `.` and `..`, so
 *	that the final output will be the minimum path.
 *
 *	If the relative path parameter to be applied is actually an absolute path,
 *	then the base path will be abandoned and be treated as "/" instead. This
 *	function can also be used to sanitize and simplify a path by applying the
 *	input path to "/".
 *
 *	@note This function is implemented with string manipulations only, and
 *		  invalid paths will not trigger and error. If you want to do an actual
 *		  file system walk or check access permission through the walk, do NOT
 *		  use this function.
 *
 *	@param path: the base path. Will be modified.
 *	@param relpath: the relative path to be applied on `path`
 *	@return 0 on success, or the negative of an errno on failure.
 */
int path_cd(pathname_t path, const char *relpath);

#endif
