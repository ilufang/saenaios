/**
 *	@file signal.h
 *
 *	Software signal facilities
 */
#ifndef SIGNAL_H
#define SIGNAL_H

#include "sys/types.h"
#include "stdint.h"

#define SIGHUP		1	///< terminal line hangup
#define SIGINT		2	///< interrupt program
#define SIGQUIT		3	///< quit program
#define SIGILL		4	///< illegal instruction
#define SIGTRAP		5	///< trace trap
#define SIGABRT		6	///< abort program (formerly SIGIOT)
#define SIGEMT		7	///< emulate instruction executed
#define SIGFPE		8	///< floating-point exception
#define SIGKILL		9	///< kill program
#define SIGBUS		10	///< bus error
#define SIGSEGV		11	///< segmentation violation
#define SIGSYS		12	///< non-existent system call invoked
#define SIGPIPE		13	///< write on a pipe with no reader
#define SIGALRM		14	///< real-time timer expired
#define SIGTERM		15	///< software termination signal
#define SIGURG		16	///< urgent condition present on socket
#define SIGSTOP		17	///< stop (cannot be caught or ignored)
#define SIGTSTP		18	///< stop signal generated from
#define SIGCONT		19	///< continue after stop
#define SIGCHLD		20	///< child status has changed
#define SIGTTIN		21	///< background read attempted from control terminal
#define SIGTTOU		22	///< background write attempted to control terminal
#define SIGIO		23	///< I/O is possible on a descriptor
#define SIGXCPU		24	///< cpu time limit exceeded
#define SIGXFSZ		25	///< file size limit exceeded
#define SIGVTALRM	26	///< virtual time alarm
#define SIGPROF		27	///< profiling timer alarm
#define SIGWINCH	28	///< Window size change
#define SIGINFO		29	///< status request from keyboard
#define SIGUSR1		30	///< User defined signal 1
#define SIGUSR2		31	///< User defined signal 2
#define SIG_MAX		32	///< Total number of signals

#define SIG_ERR		((void (*)(int)) -1) ///< Error
#define SIG_DFL		((void (*)(int)) 0) ///< Preset 'Default' handler
#define SIG_IGN		((void (*)(int)) 1)	///< Preset 'Ignore' handler
#define SIG_391CHLD	((void (*)(int)) 2)	///< 391 SIGCHLD handler (Do not use)

/// Signal set
typedef uint32_t sigset_t;

/// Add signal to set
#define sigaddset(set, signo) (*(set) |= (1<<(signo)))
/// Delete signal from set
#define sigdelset(set, signo) (*(set) &= ~(1<<(signo)))
/// Deselect all signals
#define sigemptyset(set) (*(set) = 0)
/// Select all signals
#define sigfillset(set) (*(set) = -1)
/// Test signal existence in set
#define sigismember(set, signo) (*(set) & (1<<(signo)))

#define SIG_BLOCK	1	///< Block signals in set
#define SIG_UNBLOCK	2	///< Unblock signals in set
#define SIG_SETMASK	3	///< Set mask to set

/// Do not send SIGCHLD to the parent when the process is stopped
#define SA_NOCLDSTOP	0x1
/// Do not create a zombie when the process terminates
#define SA_NOCLDWAIT	0x2
/// Ignored. (Use an alternative stack for the signal handler)
#define SA_ONSTACK		0x4
/// Interrupted system calls are automatically restarted
#define SA_RESTART		0x8
/// Ignored. (Do not mask the signal while executing the signal handler)
#define SA_NODEFER		0x10
/// Reset to default action after executing the signal handler
#define SA_RESETHAND	0x20
/// Send signal number to handler in ECE391 format
#define SA_ECE391SIGNO	0x40

/**
 * Signal handler
 *
 *	@param sig: the signal number
 */
typedef void (*sig_t)(int sig);

/**
 *	Signal Handling behavior descriptor
 */
struct sigaction {
	/**
	 *	The signal handler
	 *
	 *	@param sig: the signal number
	 */
	sig_t handler;
	/// Flags. See `SA_*` macro definitions
	uint32_t flags;
	/// Bitmap of signals to be masked during execution of the handler
	sigset_t mask;
};

/**
 *	Set signal handler
 *
 *	@param sig: the signal to set
 *	@param handler: the handler to process `sig`
 *	@return the previously-installed handler, or SIG_ERR on failure. Set errno
 */
sig_t signal(int sig, sig_t handler);

/**
 *	Get or set signal handler
 *
 *	@param sig: the signal number
 *	@param act: the new handler. Will be installed if not NULL
 *	@param oldact: buffer to read current handler into. Will be populated if not
 *		   NULL.
 *	@return 0 on success, or the -1 on failure. Set errno
 */
int sigaction(int sig, const struct sigaction *act, struct sigaction *oldact);

/**
 *	Get or set signal mask
 *
 *	@param how: action to be performed. See `SIG_*` macro definitions
 *	@param set: new signal set
 *	@param oldset: buffer to read current signal set into
 *	@return 0 on success, or -1 on failure. Set errno
 */
int sigprocmask(int how, sigset_t *set, sigset_t *oldset);

/**
 *	Suspend execution until a signal
 *
 *	@param sigset: signal set of masked signals
 *	@return always -EINTR
 */
int sigsuspend(sigset_t *sigset);

/**
 *	Send signal to process
 *
 *	@param pid: the pid of the recipient process
 *	@param sig: the signal to send
 *	@return 0 on success, or -1 on failure. Set errno
 */
int kill(pid_t pid, int sig);

#endif
