/**
 *	@file proc/task.h
 *
 *	Data structures tracking tasks/processes.
 */
#ifndef PROC_TASK_H
#define PROC_TASK_H

#include "../types.h"
#include "../fs/vfs.h"
#include "../libc.h"
#include "../boot/syscall.h"
#include "../boot/ece391_syscall.h"

#define TASK_ST_NA			0	///< Process PID is not in use
#define TASK_ST_RUNNING		1	///< Process is actively running on processor
#define TASK_ST_SLEEP		2	///< Process is sleeping
#define TASK_ST_ZOMBIE		3	///< Process is awaiting parent `wait()`
#define TASK_ST_DEAD		4	///< Process is dead

#define TASK_MAX_PROC		32767	///< Maximum of concurrently-scheduled tasks
#define TASK_MAX_OPEN_FILES	16		///< Per-process limit of concurrent open files

/**
 *	Structure for a process in the PID table
 *
 *	@todo Scheduler may want to insert saved state
 *	@todo Fork and Execve may want to insert memory map
 *	@todo VFS security may want to insert identity information
 */
typedef struct s_task {
	pid_t pid;			///< current process id
	pid_t parent;		///< parent process id
	int32_t esp;		///< esp pointer
	int32_t ebp;		///< ebp pointer
	int32_t flags;		///< flags stored for current process
	uint8_t status; 	///< Current status of this task
	file_t *files[TASK_MAX_OPEN_FILES]; ///< File descriptor pool
	char args[128];		///< @deprecated Should be placed on user stack
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
 *	@todo Will be removed later
 */
void task_create_kernel_pid();

/**
 *	Find an available pid
 *
 *	@return the new pid, or -EAGAIN if all pids are in use (probably very bad)
 */
int16_t task_alloc_pid();

#endif
