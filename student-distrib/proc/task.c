#include "task.h"

#include "../fs/vfs.h"

task_t* task_list[TASK_MAX_PROC];
file_t* stdin_file;
file_t* stdout_file;

pid_t task_current_pid() {
	task_t* curr_task = get_curr_task();
	return (pid_t)(curr_task->curr_pid);
}

task_t* get_curr_task(){
	task_t* task_ptr;
	// use esp register value to get pcb address
	asm volatile (
			"								\n\
			andl	%%esp, %%eax			\n\
			"
            : "=a"(task_ptr)
            : "a"(PCB_BITMASK)
            : "cc"
    );
	return task_ptr;
	
}

int32_t get_new_pid(){
	int32_t new_pid;
	for(new_pid = 0; new_pid < TASK_MAX_PROC; new_pid++){
		if(!task_list[new_pid])
			return new_pid;
	}
	// if there's no available pid left
	return -1;
}

task_t* get_task_addr(int32_t process_num){
	if(process_num<0||process_num>=TASK_MAX_PROC) return (task_t*)-1;
	return (task_t*)(8 * M_BYTE - (process_num + 1) * 8 * K_BYTE);
}
// TODO: debug workaround to hardcode stdin and stdout for kernel code

void task_create_kernel_pid() {
	task_t* init_task = get_task_addr(0);
	// should open fd 0 and 1
	init_task->parent_pid = -1;
	init_task->status = TASK_ST_RUNNING;
	task_list[0] = init_task;
	
}
