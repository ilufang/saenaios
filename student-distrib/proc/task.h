/**
 *	@file proc/task.h
 *
 *	Data structures tracking tasks/processes.
 */
#ifndef PROC_TASK_H
#define PROC_TASK_H

#include "../types.h"
#include "../fs/vfs.h"

#define TASK_ST_NA			0	///< Process PID is not in use
#define TASK_ST_RUNNING		1	///< Process is actively running on processor

#define TASK_MAX_PROC		65536 ///< Maximum of concurrently-scheduled tasks
#define TASK_MAX_OPEN_FILES	16 ///< Per-process limit of concurrent open files

/**
 *	Structure for a process in the PID table
 */
typedef struct s_task {
	int status; ///< Current status of this task
	file_t *files[TASK_MAX_OPEN_FILES]; ///< File descriptor pool
	uid_t uid; ///< User ID of the process
	gid_t gid; ///< Group ID of the process
} task_t;

/**
 *	List of processes
 */
extern task_t task_list[TASK_MAX_PROC];

/**
 *	Get the PID of the currently executing process
 *
 *	@note: this process currently always return 0 for the kernel code
 *
 *	@return the PID.
 */
pid_t task_current_pid();

/**
 *	TEMPORARY: initialize pid 0 for kernel code
 *
 *	Will be removed later
 */
void task_create_kernel_pid();

#endif
