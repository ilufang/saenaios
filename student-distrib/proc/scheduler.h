#ifndef SCHEDULER_H_

#define SCHEDULER_H_

#define STACK_UNKO_MAGIC 1145141919

#include "task.h"

#include "../x86_desc.h"

/**
 *	this function is called by rtc interrupt handler
 *	to start task switch
 *
 *	It determines which task to switch to
 */
void scheduler_event();

/**
 *	this function is called by scheduler event to do the real
 *	task switch
 *	
 *	@param from: the process to be switched out
 *	@param to: the process to be switched to
 *	
 *	@note this function will never actually "return"
 */
void scheduler_switch(task_t* from, task_t* to);

/**
 *	this function tear down page table entries accordingly
 *	
 *	@param pages: list of page entries to delete
 */
void scheduler_page_clear(task_ptentry_t* pages);

/**
 *	this function set up page table entries accordingly
 *	
 *	@param pages: list of page entries to setup
 */
void scheduler_page_setup(task_ptentry_t* pages);

regs_t*	scheduler_get_magic();

void scheduler_iret(
	int ebx, int ecx, int edx, int esi, int edi, int ebp, int eax,
	int return_addr, int cs, int eflags, int esp, int ss);

#endif SCHEDULER_H_
