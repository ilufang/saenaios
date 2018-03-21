#include "task.h"

#include "../fs/vfs.h"

task_t task_list[TASK_MAX_PROC];

pid_t task_current_pid() {
	return 0;
}

// TODO: debug workaround to hardcode stdin and stdout for kernel code

inode_t inode_stdin, inode_stdout;
file_t file_stdin, file_stdout;
file_operations_t fop_stdin, fop_stdout;

void task_create_kernel_pid() {
	task_list[0].status = TASK_ST_RUNNING;
	inode_stdin.open_count = 1;
	inode_stdout.open_count = 1;
	inode_stdin.f_op = &fop_stdin;
	inode_stdout.f_op = &fop_stdout;
	file_stdin.open_count = 1;
	file_stdout.open_count = 1;
	file_stdin.inode = &inode_stdin;
	file_stdout.inode = &inode_stdout;
	file_stdin.pos = 0;
	file_stdout.pos = 0;
	file_stdin.f_op = &fop_stdin;
	file_stdout.f_op = &fop_stdout;
}
