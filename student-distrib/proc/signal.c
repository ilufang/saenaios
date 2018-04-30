#include "signal.h"

#include "task.h"
#include "scheduler.h"
#include "../lib.h"
#include "../boot/page_table.h"
#include "signal_user.h"
#include "../../libc/src/syscalls.h"
#include "../../libc/include/sys/wait.h"

char *signal_names[] = {
	"Unknown signal: 0\n",
	"Hangup: 1\n",
	"Interrupt: 2\n",
	"Quit: 3\n",
	"Illegal instruction: 4\n",
	"Trace/BPT trap: 5\n",
	"Abort trap: 6\n",
	"EMT trap: 7\n",
	"Floating point exception: 8\n",
	"Killed: 9\n",
	"Bus error: 10\n",
	"Segmentation fault: 11\n",
	"Bad system call: 12\n",
	"Broken pipe: 13\n",
	"Alarm clock: 14\n",
	"Terminated: 15\n",
	"Urgent I/O condition: 16\n",
	"Suspended (signal): 17\n",
	"Suspended: 18\n",
	"Continued: 19\n",
	"Child exited: 20\n",
	"Stopped (tty input): 21\n",
	"Stopped (tty output): 22\n",
	"I/O possible: 23\n",
	"Cputime limit exceeded: 24\n",
	"Filesize limit exceeded: 25\n",
	"Virtual timer expired: 26\n",
	"Profiling timer expired: 27\n",
	"Window size changes: 28\n",
	"Information request: 29\n",
	"User defined signal 1: 30\n",
	"User defined signal 2: 31\n",
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
		sigdelset(&(proc->signal_mask), SIGKILL);
		sigdelset(&(proc->signal_mask), SIGSTOP);
	}

	return 0;
}

int syscall_sigsuspend(int sigsetp, int b, int c) {
	task_t *proc;

	proc = task_list + task_current_pid();

	if (sigsetp) {
		proc->signal_mask = *(sigset_t *)sigsetp & ~(SIGKILL | SIGSTOP);
	}
	// proc->regs.eax = -EINTR;
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

	sigaddset(&(proc->signals), sig);

	return 0;
}

/// Map ECE391 signal number to POSIX signal number
int signal_ece391_map[] = {
	SIGFPE,		// DIV_ZERO
	SIGSEGV,	// SEGFAULT
	SIGTERM,	// INTERRUPT
	SIGALRM,	// ALARM
	SIGUSR1		// USER1
};

int syscall_ece391_set_handler(int sig, int handlerp, int c) {
	task_sigact_t sa;

	if (sig < 0 || sig >= 5) {
		return -EINVAL;
	}
	sig = signal_ece391_map[sig];
	sa.handler = (void (*)(int))handlerp;
	sigemptyset(&(sa.mask));
	sigaddset(&(sa.mask), sig);
	sa.flags = SA_ECE391SIGNO;
	return syscall_sigaction(sig, (int)&sa, 0);
}

int syscall_ece391_sigreturn(int a, int b, int c) {
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
	int ret;

	sa = proc->sigacts + sig;

	switch((int)(sa->handler)) {
		case ((int)SIG_DFL):
			signal_exec_default(proc, sig);
			return;
		case ((int)SIG_391CHLD):
			if (sig == SIGCHLD) {
				signal_handler_child(proc);
				return;
			}
			// If not, probably very bad (abuse)
			// Fall thru
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
	// Workaround ECE391 expecting EAX at arg1 + 7
	proc->regs.esp_k = proc->regs.eax;
	task_user_pushs(&(proc->regs.esp), (uint8_t *)&(proc->regs), 36); // Only the 8 GPRs
	task_user_pushl(&(proc->regs.esp), proc->regs.eflags);
	// Push original mask
	task_user_pushl(&(proc->regs.esp), proc->signal_mask);
	proc->signal_mask = sa->mask & (~(SIGKILL | SIGSTOP));
	// Push stack frame for handler
	if (sa->flags & SA_ECE391SIGNO) {
		switch(sig) {
			case SIGFPE:
				task_user_pushl(&(proc->regs.esp), 0);
				break;
			case SIGSEGV:
				task_user_pushl(&(proc->regs.esp), 1);
				break;
			case SIGTERM:
				task_user_pushl(&(proc->regs.esp), 2);
				break;
			case SIGALRM:
				task_user_pushl(&(proc->regs.esp), 3);
				break;
			case SIGUSR1:
				task_user_pushl(&(proc->regs.esp), 4);
				break;
			default:
				task_user_pushl(&(proc->regs.esp), sig);
		}
	} else {
		task_user_pushl(&(proc->regs.esp), sig);
	}
	ret = task_user_pushl(&(proc->regs.esp), (uint32_t)signal_user_ret_addr);
	if (ret != 0) {
		// Stack overflow... BAD!
		signal_handler_terminate(proc, SIGSEGV);
		return; // This line should not hit
	}
	// Jump to handler
	proc->regs.eip = (uint32_t) sa->handler;
}

void signal_exec_default(task_t *proc, int sig) {
	switch(sig) {
		case SIGCHLD:
		case SIGURG:
		case SIGCONT:
		case SIGIO:
		case SIGWINCH:
		case SIGINFO:
		// The next 3 signals should have been terminated, but is currently set
		// to be ignored for ece391 signal handling logic
		case SIGALRM:
		case SIGUSR1:
		case SIGUSR2:
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

void signal_handler_child(task_t *proc) {
	int i;

	// Gather child information
	for (i = 0; i < TASK_MAX_PROC; i++) {
		if (task_list[i].parent == proc->pid &&
			task_list[i].status == TASK_ST_ZOMBIE) {
			if (proc->status == TASK_ST_SLEEP && WIFSYSCALL(proc->exit_status)&&
				WBLOCKSYSNO(proc->exit_status) == 1) {
				// Only send child info if the parent is ece391_execute
				if (WIFSIGNALED(task_list[i].exit_status)) {
					proc->regs.eax = 256;
				} else if (WIFEXITED(task_list[i].exit_status)) {
					// Sign extend
					proc->regs.eax = (int8_t)WEXITSTATUS(task_list[i].exit_status);
				} else {
					// This is probably bad
					proc->regs.eax = task_list[i].exit_status;
				}
			}
			task_release(task_list + i);
			break;
		}
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
	//syscall_write(1, (int)signal_names[sig], strlen(signal_names[sig]));
	(*(proc->files[1]->f_op->write))(proc->files[1],(uint8_t*)signal_names[sig],strlen(signal_names[sig]),0);

	// Let process execute _exit
	proc->regs.eax = SYSCALL__EXIT;
	proc->regs.ebx = sig | WIFSIGNALED(-1);
	proc->regs.eip = (uint32_t) signal_user_make_syscall_addr;
}

void signal_handler_stop(task_t *proc, int sig) {
	proc->status = TASK_ST_SLEEP;
	proc->exit_status = sig | WIFSTOPPED(-1);
}
