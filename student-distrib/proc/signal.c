#include "signal.h"

#include "task.h"
#include "scheduler.h"
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

	if (sig == SIGKILL || sig == SIGSTOP) {
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

int syscall_sigprocmask(int how, int setp, int oldsetp) {
	task_t *proc;
	sigset_t *ss;

	proc = task_list + task_current_pid();

	if (oldsetp) {
		// TODO fully validate user pointer
		ss = (sigset_t *) oldsetp;
		*ss = proc->signal_mask;
	}

	if (setp) {
		// TODO fully validate user pointer
		ss = (sigset_t *) setp;

		switch(how) {
			case SIG_BLOCK:
				proc->signal_mask &= *ss;
				break;
			case SIG_UNBLOCK:
				proc->signal_mask |= ~*ss;
				break;
			case SIG_SETMASK:
				proc->signal_mask = *ss;
				break;
			default:
				return -EINVAL;
		}
		// Enforce SIGKILL and SIGSTOP
		sigdelset(proc->signal_mask, SIGKILL);
		sigdelset(proc->signal_mask, SIGSTOP);
	}

	return 0;
}

int syscall_sigsuspend(int sigsetp, int b, int c) {
	task_t *proc;

	proc = task_list + task_current_pid();

	if (sigsetp) {
		proc->signal_mask = *(sigset_t *)sigsetp & ~(SIGKILL | SIGSTOP);
	}
	proc->regs.eax = -EINTR;
	proc->status = TASK_ST_SLEEP;

	scheduler_event();
	return 0; // Should not hit
}

int syscall_kill(int pid, int sig, int c) {
	task_t *proc;

	if (pid <= 0 || pid >= TASK_MAX_PROC) {
		return -EINVAL;
	}
	if (sig <= 0 || sig >= SIG_MAX) {
		return -EINVAL;
	}

	proc = task_list + pid;
	if (proc->status != TASK_ST_RUNNING && proc->status != TASK_ST_SLEEP) {
		return -ESRCH;
	}

	sigaddset(proc->signals, sig);

	return 0;
}

void signal_init() {
	uint32_t paddr = 0;
	page_alloc_4KB((int *)&paddr);
	page_tab_add_entry(PROC_USR_BASE, paddr, PAGE_TAB_ENT_PRESENT |
					   PAGE_TAB_ENT_RDWR | PAGE_TAB_ENT_USER |
					   PAGE_TAB_ENT_GLOBAL);
	page_flush_tlb();
	memcpy((uint8_t *) PROC_USR_BASE, &(signal_user_base), signal_user_length);

}

void signal_exec(task_t *proc, int sig) {
	task_sigact_t *sa;

	proc = task_list + task_current_pid();

	// Resume program execution
	proc->status = TASK_ST_RUNNING;

	sa = proc->sigacts + sig;

	switch((int)(sa->handler)) {
		case ((int)SIG_DFL):
			signal_exec_default(proc, sig);
			return;
		case ((int)SIG_IGN):
			signal_handler_ignore(proc, sig);
			return;
	}

	// Custom handler provided, execute that

	// Push stack frame for signal_user_ret
	if (sa->flags & SA_RESTART) {
		// Restart INT 0x80
		// TODO validity check
		if (*(uint8_t *)(proc->regs.eip - 2) == 0xcd) { // OPCode for INT: CD
			task_user_pushl(&(proc->regs.esp), proc->regs.eip - 2);
		} else {
			task_user_pushl(&(proc->regs.esp), proc->regs.eip);
		}
	} else {
		task_user_pushl(&(proc->regs.esp), proc->regs.eip);
	}
	task_user_pushs(&(proc->regs.esp), (uint8_t *)&(proc->regs), 32); // Only the 8 GPRs
	task_user_pushl(&(proc->regs.esp), proc->regs.eflags);
	// Push original mask
	task_user_pushl(&(proc->regs.esp), proc->signal_mask);
	proc->signal_mask = sa->mask & (~(SIGKILL | SIGSTOP));
	// Push stack frame for handler
	task_user_pushl(&(proc->regs.esp), (uint32_t)signal_user_ret_addr);
	if (task_user_pushl(&(proc->regs.esp), sig) != 0) {
		// Stack overflow... BAD!
		signal_handler_terminate(proc, SIGSEGV);
		return; // This line should not hit
	}
	// Jump to handler
	proc->regs.eip = (uint32_t) sa->handler;
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
	if (proc->sigacts[sig].flags & SA_RESTART) {
		// Restart INT 0x80
		// TODO validity check
		if (*(uint8_t *)(proc->regs.eip - 2) == 0xcd) { // OPCode for INT: CD
			proc->regs.eip -= 2;
		}
	}
}

void signal_handler_terminate(task_t *proc, int sig) {
	// Print to stdout
	syscall_write(1, (int)signal_names[sig], strlen(signal_names[sig]));
	// Halt process
	syscall__exit(1, 0, 0);
}

void signal_handler_stop(task_t *proc, int sig) {
	proc->status = TASK_ST_SLEEP;
}
