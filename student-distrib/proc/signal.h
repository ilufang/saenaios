/**
 *	@file proc/signal.h
 *
 *	Process signals
 */
#ifndef PROC_SIGNAL_H
#define PROC_SIGNAL_H

#include "../../libc/include/signal.h"
#include "task.h"

typedef struct sigaction task_sigact_t;

/**
 *	Get or set signal handler
 *
 *	@param sig: the signal number
 *	@param actp: pointer to `task_sigact_t *`: the new handler
 *	@param oldactp: pointer to `task_sigact_t *`:buffer to read current
 *		   handler into.
 *	@return 0 on success, or the negative of an errno on failure.
 */
int syscall_sigaction(int sig, int actp, int oldactp);

/**
 *	Invoke signal handler on current process.
 *
 *	@param sig: the signal number
 */
void signal_exec(int sig);

/**
 *	Execute initializations needed by the signal invocation systems.
 */
void signal_init();

/**
 *	Invoke the default signal handler.
 *
 *	@param proc: the process
 *	@param sig: the signal number
 */
void signal_exec_default(task_t *proc, int sig);

/**
 *	Handle signal by simply discarding the signal.
 *
 *	@param proc: the process
 *	@param sig: the signal number
 */
void signal_handler_ignore(task_t *proc, int sig);

/**
 *	Handle signal by terminating process. Will also print an error message
 *
 *	@param proc: the process
 *	@param sig: the signal number
 */
void signal_handler_terminate(task_t *proc, int sig);

/**
 *	Handle signal by putting the process to sleep.
 *
 *	@param proc: the process
 *	@param sig: the signal number
 */
void signal_handler_stop(task_t *proc, int sig);

#endif
