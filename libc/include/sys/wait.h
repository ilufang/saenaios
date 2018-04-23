/**
 *	@file sys/wait.h
 *
 *	Utilities to wait for processes
 */
#ifndef SYS_WAIT_H
#define SYS_WAIT_H

/**
 *	Wait for child process to terminate.
 *
 *	This call will collect return codes from terminated child process (aka.
 *	zombies). If none of the child processes has terminated, this call will
 *	sleep the calling process and block execution until a child terminate. If
 *	the calling process does not have any child processes, this call will return
 *	`-ECHILD`.
 *
 *	@param status: pointer to an `int` buffer. The child's return code will be
 *		   populated into this buffer
 *	@return the pid of the terminated child process, or the negative of an errno
 *			on failure.
 */
pid_t wait(int *status);

#endif
