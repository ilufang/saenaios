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
 *	Get or set signal mask
 *
 *	@param how: action to be performed. See `SIG_*` macro definitions
 *	@param setp: address of new signal set
 *	@param oldsetp: address of buffer to read current signal set into
 *	@return 0 on success, or the negative of an errno on failure
 */
int syscall_sigprocmask(int how, int setp, int oldsetp);

/**
 *	Suspend execution until a signal
 *
 *	@param sigsetp: address of signal set of masked signals
 *	@return always -EINTR
 */
int syscall_sigsuspend(int sigsetp, int, int);

/**
 *	Send signal to process
 *
 *	@param pid: the pid of the recipient process
 *	@param sig: the signal to send
 *	@return 0 on success, or the negative of an errno on failure
 */
int syscall_kill(int pid, int sig, int);

/**
 *	Invoke signal handler.
 *
 *	@param proc: the process
 *	@param sig: the signal number
 */
void signal_exec(task_t *proc, int sig);

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
 *	Default SIGCHLD handler
 *
 *	@param proc: the process
 */
void signal_handler_child(task_t *proc);

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


extern void* task_kernel_process;

extern int task_kernel_process_length;

/**
 *	Asm function to iret to user level kernel init process
 *
 */
void task_kernel_process_iret();

#endif
