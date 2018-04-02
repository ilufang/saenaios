#include "pcb.h"

#include "../fs/vfs.h"
/* 8kb-10 0000 0000 0000 */
#define PCB_BITMASK		0xFFFFE000		///< bitmask to reach the top of process's 8kb stack

pcb_t* proc_list[PCB_MAX_PROC];
static pcb_t* active_pcb;

// clear all existing processes and create the first task
void proc_init(){
	int i = 0;
	for(i = 0; i < PCB_MAX_PROC; i++){
		proc_list[i] = NULL;	
	}
	pcb_t* init_pcb = get_pcb_addr(0);
	proc_list[0] = init_pcb;
	set_active_pcb(0);
	int32_t fd_in, fd_out;
	fd_in = open("/dev/stdin", O_RDONLY, 0);
	fd_out = open("/dev/stdout", O_WRONLY, 0);
	init_pcb->parent_pid = -1;
}

pcb_t* get_pcb_addr(int32_t process_num){
	return (pcb_t*)(8 * M_BYTE - (process_num + 1) * 8 * K_BYTE);
}
	
pcb_t* get_curr_pcb(){
	pcb_t* pcb_ptr;
	asm volatile (
			"								\n\
			andl	%%esp, %%eax			\n\
			"
            : "=a"(pcb_ptr)
            : "a"(PCB_BITMASK)
            : "cc"
    );
	return pcb_ptr;
}

void set_active_pcb(int pid){
	active_pcb = get_pcb_addr(pid);
}

pcb_t* get_active_pcb(){
	return active_pcb;
}

int32_t get_new_pid(){
	int32_t new_pid;
	for(new_pid = 0; new_pid < PCB_MAX_PROC; new_pid++){
		if(!proc_list[new_pid])
			return new_pid;
	}
	// if there's no available pid left
	return -1;
}
