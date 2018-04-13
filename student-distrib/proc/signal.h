/**
 *	@file proc/signal.h
 *
 *	Process signals
 */
#ifndef PROC_SIGNAL_H
#define PROC_SIGNAL_H

#include "../../libc/include/signal.h"

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



#endif
