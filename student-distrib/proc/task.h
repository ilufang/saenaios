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

#define TASK_MAX_PROC		65536 ///< Maximum of concurrently-scheduled tasks
#define TASK_MAX_OPEN_FILES	16 ///< Per-process limit of concurrent open files

#define M_BYTE			0x100000	///< offset for 1MB
#define K_BYTE			0x400		///< offset for 1kb
/**
 *	Structure of saved registers produced by `pusha`
 */
typedef struct s_regs {
	uint32_t magic； ///< Should be 1145141919
	uint32_t flags; ///< Saved eflags
	uint32_t edi; ///< Saved edi
	uint32_t esi; ///< Saved esi
	uint32_t ebp; ///< Saved ebp
	uint32_t esp; ///< Saved esp
	uint32_t ebx; ///< Saved ebx
	uint32_t edx; ///< Saved edx
	uint32_t ecx; ///< Saved ecx
	uint32_t eax; ///< Saved eax
} regs_t;

/* 8kb-10 0000 0000 0000 */
#define PCB_BITMASK		0xFFFFE000		///< bitmask to reach the top of process's 8kb stack

#define MAX_ARGS		128				///< arguments stored for the current process

/**
 *	Structure for a process in the PID table
 */
typedef struct s_task {
	int32_t curr_pid;		///< current process id
	int32_t parent_pid;		///< parent process id
	int32_t esp;			///< esp pointer
	int32_t ebp;			///< ebp pointer
	int32_t flags;			///< flags stored for current process
	int32_t status; ///< Current status of this task
	uint8_t args[MAX_ARGS];
	file_t *files[TASK_MAX_OPEN_FILES]; ///< File descriptor pool
} task_t;

/**
 *	List of processes
 */
extern task_t* task_list[TASK_MAX_PROC];
/*
extern file_t* stdin_file;
extern file_t* stdout_file;
*/
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
 *	Will be removed later
 */
void task_create_kernel_pid();


/**
 *	Get pointer to current task
 *
 *	And esp with bitmask to get the pcb pointer on top of 8kb stack
 */
task_t* get_curr_task();


/**
 *	Address calculation of a task pointer
 *
 *	Calculate address of the pcb pointer given pid
 *	@param process_num: pid of the process
 */
task_t* get_task_addr(int32_t process_num);

/**
 *	Generates a new pid
 *
 *	Find an available pid
 *	@return the new pid
 */
int32_t get_new_pid();

#endif
