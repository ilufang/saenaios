/**
 *	@file proc/task.h
 *
 *	Data structures tracking tasks/processes.
 */
#ifndef PROC_TASK_H
#define PROC_TASK_H

#include "../types.h"
#include "../fs/vfs.h"

#define TASK_ST_NA			0	
#define TASK_ST_RUNNING		1

#define TASK_MAX_PROC		65536
#define TASK_MAX_OPEN_FILES	16

typedef uint16_t pid_t;

/**
 *	Structure for a process in the PID table
 */
typedef struct s_task {
	int status; ///< Current status of this task
	file_t *files[TASK_MAX_OPEN_FILES];
} task_t;

extern task_t task_list[TASK_MAX_PROC];

pid_t task_current_pid();

void task_create_kernel_pid();

#endif
