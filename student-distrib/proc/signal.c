#include "signal.h"

#include "task.h"
#include "../lib.h"

int syscall_sigaction(int sig, int actp, int oldactp) {
	task_t *proc;
	task_sigact_t *act;

	if (sig < 0 || sig > SIG_MAX) {
		return -EINVAL;
	}

	proc = task_list + task_current_list();

	if (oldactp) {
		// Copy from task to user
		act = (task_sigact_t *) oldactp;
		// TODO: fully validate user pointer
		memcpy(act, proc->sigacts[sig], sizeof(task_sigact_t));
	}

	if (actp) {
		// Copy from user to task
		act = (task_sigact_t *) actp;
		// TODO: fully validate user pointer
		memcpy(proc->sigacts[sig], act, sizeof(task_sigact_t));
	}

	return 0;
}
