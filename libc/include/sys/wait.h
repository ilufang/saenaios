/**
 *	@file sys/wait.h
 *
 *	Utilities to wait for processes
 */
#ifndef SYS_WAIT_H
#define SYS_WAIT_H

#include "types.h"

/// wait call should not block if there are no processes pending status report
#define WNOHANG		0x1
/// wait also collects from child stops
#define WUNTRACED	0x2

/// True if the process terminated normally
#define WIFEXITED(status) (status & 0x100)
/// True if the process terminated due to receipt of a signal
#define WIFSIGNALED(status) (status & 0x200)
/// True if the process has stopped
#define WIFSTOPPED(status) (status & 0x400)
/// True if the process slept because of a blocking syscall
#define WIFSYSCALL(status) (status & 0x800)
/// Extracts the exit code if the process has been terminated
#define WEXITSTATUS(status) (status & 0xff)
/// Extracts the responsible signal if the process has been signal-terminated
#define WTERMSIG(status) (status & 0xff)
/// Evaluates to zero. No core dumps will occur
#define WCOREDUMP(status) 0
/// Extracts the responsible signal if the process has been stopped
#define WSTOPSIG(status) (status & 0xff)
/// Extracts the syscall number that put the process to sleep
#define WBLOCKSYSNO(status) (status & 0xff)

/**
 *	Wait for any child process to change status.
 *
 *	`wait` is essentially calling `waitpid(-1, status, )`.
 *
 *	@param status: pointer to an `int` buffer. The child's status will be
 *		   populated into this buffer
 *	@return the pid of the terminated child process, or the negative of an errno
 *			on failure.
 */
pid_t wait(int *status);

/**
 *	Wait for child process(es) to change status
 *
 *	This call will collect return codes from terminated child process (aka.
 *	zombies). If none of the child processes has terminated, this call will
 *	sleep the calling process and block execution until a child terminate. If
 *	the calling process does not have any child processes, this call will return
 *	`-ECHILD`.
 *
 *	@param pid: the pid to wait for, or -1 to wait for all child processes
 *	@param status: pointer to an `int` buffer. The child's status will be
 *		   populated into this buffer
 *	@param options: bit map of option flags
 */
pid_t waitpid(pid_t pid, int *status, int options);

#endif
