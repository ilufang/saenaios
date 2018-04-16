#include "signal.h"

#include "task.h"
#include "../lib.h"
#include "../boot/page_table.h"
#include "signal_user.h"

char *signal_names[] = {
	"Unknown signal: 0",
	"Hangup: 1",
	"Interrupt: 2",
	"Quit: 3",
	"Illegal instruction: 4",
	"Trace/BPT trap: 5",
	"Abort trap: 6",
	"EMT trap: 7",
	"Floating point exception: 8",
	"Killed: 9",
	"Bus error: 10",
	"Segmentation fault: 11",
	"Bad system call: 12",
	"Broken pipe: 13",
	"Alarm clock: 14",
	"Terminated: 15",
	"Urgent I/O condition: 16",
	"Suspended (signal): 17",
	"Suspended: 18",
	"Continued: 19",
	"Child exited: 20",
	"Stopped (tty input): 21",
	"Stopped (tty output): 22",
	"I/O possible: 23",
	"Cputime limit exceeded: 24",
	"Filesize limit exceeded: 25",
	"Virtual timer expired: 26",
	"Profiling timer expired: 27",
	"Window size changes: 28",
	"Information request: 29",
	"User defined signal 1: 30",
	"User defined signal 2: 31",
	"" // dummy
};


int syscall_sigaction(int sig, int actp, int oldactp) {
	task_t *proc;
	task_sigact_t *act;

	if (sig < 0 || sig > SIG_MAX) {
		return -EINVAL;
	}

	proc = task_list + task_current_pid();

	if (oldactp) {
		// Copy from task to user
		act = (task_sigact_t *) oldactp;
		// TODO: fully validate user pointer
		memcpy(act, proc->sigacts+sig, sizeof(task_sigact_t));
	}

	if (actp) {
		// Copy from user to task
		act = (task_sigact_t *) actp;
		// TODO: fully validate user pointer
		memcpy(proc->sigacts+sig, act, sizeof(task_sigact_t));
	}

	return 0;
}

void signal_init() {
	uint32_t paddr;
	page_alloc_4KB((int *)&paddr);
	page_tab_add_entry(0x8000000, paddr, PAGE_TAB_ENT_PRESENT |
					   PAGE_TAB_ENT_RDONLY | PAGE_TAB_ENT_USER |
					   PAGE_TAB_ENT_GLOBAL);
	memcpy((uint8_t *) PROC_USR_BASE, &(signal_user_base), signal_user_length);
}

void signal_exec(int sig) {
	task_t *proc;

	proc = task_list + task_current_pid();

	// Resume program execution
	proc->status = TASK_ST_RUNNING;

	switch((int)proc->sigacts[sig].handler) {
		case SIG_DFL:
			return signal_exec_default(proc, sig);
		case SIG_IGN:
			// TODO perform context switch
			return;
	}

	// Custom handler provided, execute that
	// Push stack frame for signal_user_ret
	task_user_pushl(&(proc->regs.esp), proc->regs.eip);
	task_user_pushs(&(proc->regs.esp), (uint8_t *)&(proc->regs), 32); // Only the 8 GPRs
	task_user_pushl(&(proc->regs.esp), proc->regs.eflags);
	// Push stack frame for handler
	task_user_pushl(&(proc->regs.esp), (uint32_t)signal_user_ret_addr);
	if (task_user_pushl(&(proc->regs.esp), sig) != 0) {
		// Stack overflow... BAD!
		signal_handler_terminate(proc, SIGSEGV);
		return; // This line should not hit
	}

	// Signal options...

	// TODO perform context switch to user mode
}

void signal_exec_default(task_t *proc, int sig) {
	switch(sig) {
		case SIGURG:
		case SIGCONT:
		case SIGCHLD:
		case SIGIO:
		case SIGWINCH:
		case SIGINFO:
			signal_handler_ignore(proc, sig);
			break;
		case SIGSTOP:
		case SIGTSTP:
		case SIGTTIN:
		case SIGTTOU:
			signal_handler_stop(proc, sig);
			break;
		default:
			signal_handler_terminate(proc, sig);
	}
}

void signal_handler_ignore(task_t *proc, int sig) {
	// TODO perform context switch
}

void signal_handler_terminate(task_t *proc, int sig) {
	// Print to stdout
	syscall_write(1, (int)signal_names[sig], strlen(signal_names[sig]));
	// Halt process
	syscall__exit(1, 0, 0);
}

void signal_handler_stop(task_t *proc, int sig) {
	proc->status = TASK_ST_SLEEP;
	// TODO perform context switch
}
