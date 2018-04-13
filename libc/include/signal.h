/**
 *	@file signal.h
 *
 *	Software signal facilities
 */
#ifndef SIGNAL_H
#define SIGNAL_H

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

/**
 *	Signal Handling behavior descriptor
 */
struct sigaction {
	/**
	 *	The signal handler
	 *
	 *	@param sig: the signal number
	 */
	void (*sa_handler)(int sig);
};

/**
 *	Get or set signal handler
 *
 *	@param sig: the signal number
 *	@param act: the new handler. Will be installed if not NULL
 *	@param oldact: buffer to read current handler into. Will be populated if not
 *		   NULL.
 *	@return 0 on success, or the negative of an errno on failure.
 */
int sigaction(int sig, const struct sigaction *act, struct sigaction *oldact);


#endif
