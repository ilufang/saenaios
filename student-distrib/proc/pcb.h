/**
 *	@file proc/pcb.h
 *
 *	Data structures tracking tasks/processes.
 */
#ifndef PROC_PCB_H
#define PROC_PCB_H

#include "../types.h"
#include "../fs/vfs.h"
#include "task.h"
#include "../libc.h"
#include "../boot/syscall.h"

#define PROC_STATE_READY	0		///< pcb process state ready
#define PROC_STATE_RUNNING	1		///< pcb process state running
#define PROC_STATE_WAITING	2		///< pcb process state waiting

#define PCB_MAX_FILE	8			///< max file supported by a task
#define PCB_FILE_START	2			///< start index of fd after stdin and stdout
#define M_BYTE			0x100000	///< offset for 1MB
#define K_BYTE			0x400		///< offset for 1kb
#define PCB_MAX_PROC	6			///< max processes supported

/**
 *	Structure for a process control block
 */
typedef struct s_pcb {
	int32_t curr_pid;
	int32_t parent_pid;
	int32_t esp;
	int32_t ebp;
	int32_t flags;
	int32_t state;
	file_t* fd_arr[PCB_MAX_FILE]; ///< File descriptor array for current process
} pcb_t;


/**
 *	List of process
 */
extern pcb_t* proc_list[PCB_MAX_PROC];


/**
 *	Get pointer to current pcb
 *
 *	And esp with bitmask to get the pcb pointer on top of 8kb stack
 */
pcb_t* get_curr_pcb();


/**
 *	Address calculation of a pcb pointer
 *
 *	Calculate address of the pcb pointer given pid
 *	@param process_num: pid of the process
 */
pcb_t* get_pcb_addr(int32_t process_num);

/**
 *	Generates a new pid
 *
 *	Find an available pid
 *	@return the new pid
 */
int32_t get_new_pid();

/**
 *	Update the active pcb
 *
 *	@param pid: the new active pid
 *	@return 0 on success, -1 on error
 */
int set_active_pcb(int pid);

/**
 *	Return the currently active pcb
 *
 *	@return pointer to the current active pcb
 */
pcb_t* get_active_pcb();

/**
 *	Initialize the first process
 *
 *	Create the first process without memory allocation. fd 0 and 1
 *	should be automatically opened
 */
void proc_init();


#endif
