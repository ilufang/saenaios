/**
 *	@file proc/scheduler.h
 *
 *	Process scheduler
 */
#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "task.h"

#include "../x86_desc.h"

/// Set to 1 when scheduler is enabled. (Used by RTC ISR)
extern int scheduler_on_flag;

void scheduling_start();

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

/**
 *	get magic number and the regs structure behind it
 *
 *	@return pointer to the regs_t structure that current process set up
 */
regs_t*	scheduler_get_magic();

/**
 *
 *	assembly function to do actual iret to switch task
 *
 *	@param reg: the whole structure to iret
 */
void scheduler_iret(regs_t reg);

#endif
