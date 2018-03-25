/**
 *	@file sys/types.h
 *
 *	Data types
 *
 *	Reference: http://pubs.opengroup.org/onlinepubs/7908799/xsh/systypes.h.html
 */
#ifndef SYS_TYPES_H
#define SYS_TYPES_H

/// Used for file block counts
typedef unsigned long blkcnt_t;

/// Used for block sizes
typedef unsigned long blksize_t;

/// Used for system times in clock ticks or CLOCKS_PER_SEC (see <time.h>).
typedef unsigned long clock_t;

// /// Used for clock ID type in the clock and timer functions.
// TODO clockid_t;

// /// Used for timer ID returned by timer_create().
// TODO timer_t

/// Used for device IDs.
typedef unsigned short dev_t;

/// Used for file system block counts
typedef unsigned long fsblkcnt_t;

/// Used for file system file counts
typedef unsigned long fsfilcnt_t;

///  Used for group IDs.
typedef unsigned short gid_t;

/// Used for user IDs.
typedef unsigned short uid_t;

/// Used as a general identifier; can be used to contain at least a pid_t,
/// uid_t or a gid_t.
typedef unsigned long id_t;

/// Used for file serial numbers.
typedef long ino_t;

// /// Used for interprocess communication.
// TODO: key_t

/// Used for some file attributes.
typedef unsigned short mode_t;

/// Used for link counts.
typedef unsigned short nlink_t;

/// Used for file sizes.
typedef unsigned long off_t;

/// Used for process IDs and process group IDs.
typedef unsigned short pid_t;

// /// Used to identify a thread attribute object.
// TODO pthread_attr_t

// /// Used for condition variables.
// TODO pthread_cond_t

// /// Used to identify a condition attribute object.
// TODO pthread_condattr_t

// /// Used for thread-specific data keys.
// TODO pthread_key_t

// /// Used for mutexes.
// TODO pthread_mutex_t

// /// Used to identify a mutex attribute object.
// TODO pthread_mutexattr_t

// /// Used for dynamic package initialisation.
// TODO pthread_once_t

// /// Used for read-write locks.
// TODO pthread_rwlock_t

// /// Used for read-write lock attributes.
// TODO pthread_rwlockattr_t

// /// Used to identify a thread.
// TODO pthread_t

/// Used for sizes of objects.
typedef unsigned long size_t;

/// Used for a count of bytes or an error indication.
typedef long ssize_t;

/// Used for time in microseconds
typedef long suseconds_t;

/// Used for time in seconds.
typedef long time_t;

/// Used for time in microseconds.
typedef long useconds_t;

#endif
