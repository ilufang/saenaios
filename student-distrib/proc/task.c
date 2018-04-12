#include "task.h"

#include "../fs/vfs.h"

task_t task_list[TASK_MAX_PROC];
pid_t task_pid_allocator;

int16_t task_alloc_pid() {
	pid_t ret;
	for(ret = task_pid_allocator + 1; ret != task_pid_allocator; ret++) {
		if(task_list[ret].status == TASK_ST_NA) {
			task_pid_allocator = ret;
			return ret;
		}
	}
	// if there's no available pid left
	return -EAGAIN;
}

void task_create_kernel_pid() {
	// initialize the kernel task
	task_t* init_task = task_list + 0;
	// should open fd 0 and 1
	init_task->parent = -1;
	init_task->status = TASK_ST_RUNNING;
	task_pid_allocator = 0;
}
