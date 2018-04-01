#include "pcb.h"

#include "../fs/vfs.h"
#define PCB_BITMASK		0xFFFFE000		///< bitmask to reach the top of process's 8kb stack

pcb_t* proc_list[PCB_MAX_PROC];



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

int32_t get_new_pid(){
	int32_t new_pid;
	for(new_pid = 0; new_pid < PCB_MAX_PROC; new_pid++){
		if(!proc_list[new_pid])
			return new_pid;
	}
	// if there's no available pid left
	return -1;
}
