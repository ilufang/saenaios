#include "pcb.h"

#include "../fs/vfs.h"
/* 8kb-10 0000 0000 0000 */
#define PCB_BITMASK		0xFFFFE000		///< bitmask to reach the top of process's 8kb stack

pcb_t* proc_list[PCB_MAX_PROC];
static pcb_t* active_pcb;				///< the current active pcb

void proc_init(){
	// clear existing process
	int i = 0;
	for(i = 0; i < PCB_MAX_PROC; i++){
		proc_list[i] = NULL;	
	}
	// create first pcb
	pcb_t* init_pcb = get_pcb_addr(0);
	proc_list[0] = init_pcb;
	set_active_pcb(0);
	// should open fd 0 and 1
	int32_t fd_in, fd_out;
	fd_in = open("/dev/stdin", O_RDONLY, 0);
	fd_out = open("/dev/stdout", O_WRONLY, 0);
	// set parent pid to -1 because no parent
	init_pcb->parent_pid = -1;
}

pcb_t* get_pcb_addr(int32_t process_num){
	if(process_num<0||process_num>=PCB_MAX_PROC) return (pcb_t*)-1;
	return (pcb_t*)(8 * M_BYTE - (process_num + 1) * 8 * K_BYTE);
}
	
pcb_t* get_curr_pcb(){
	pcb_t* pcb_ptr;
	// use esp register value to get pcb address
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

int set_active_pcb(int pid){
	if(pid<0||pid>=PCB_MAX_PROC) return -1;
	active_pcb = get_pcb_addr(pid);
	return 0;
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
