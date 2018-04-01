/**
 *	@file sys/stat.h
 *
 *	Data returned by the stat() function
 */
#ifndef SYS_STAT_H
#define SYS_STAT_H

/// read, write, execute/search by owner
#define S_IRWXU	0700
/// read permission, owner
#define S_IRUSR	0400
/// write permission, owner
#define S_IWUSR	0200
/// execute/search permission, owner
#define S_IXUSR	0100
/// read, write, execute/search by group
#define S_IRWXG	0070
/// read permission, group
#define S_IRGRP	0040
/// write permission, group
#define S_IWGRP	0020
/// execute/search permission, group
#define S_IXGRP	0010
/// read, write, execute/search by others
#define S_IRWXO	0007
/// read permission, others
#define S_IROTH	0004
/// write permission, others
#define S_IWOTH	0002
/// execute/search permission, others
#define S_IXOTH	0001
/// set-user-ID on execution
#define S_ISUID	04000
/// set-group-ID on execution
#define S_ISGID	02000
/// on directories, restricted deletion flag
#define S_ISVTX	01000


#endif
