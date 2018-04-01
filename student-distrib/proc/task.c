#include "task.h"

#include "../fs/vfs.h"

task_t task_list[TASK_MAX_PROC];

pid_t task_current_pid() {
	return 0;
}

// TODO: debug workaround to hardcode stdin and stdout for kernel code

void task_create_kernel_pid() {
	task_list[0].status = TASK_ST_RUNNING;
	task_list[0].uid = 0; // root
	task_list[0].gid = 0; // root
}
