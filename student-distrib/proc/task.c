#include "task.h"

#include "../fs/vfs.h"
#include "elf.h"

task_t task_list[TASK_MAX_PROC];
pid_t task_pid_allocator;

int16_t task_alloc_pid() {
	pid_t ret;
	for(ret = task_pid_allocator + 1; ret != task_pid_allocator; ret++) {
		if(task_list[ret].status == TASK_ST_NA) {
			task_pid_allocator = ret;
			return ret;
		}
	}
	// if there's no available pid left
	return -EAGAIN;
}

void task_create_kernel_pid() {
	// initialize the kernel task
	task_t* init_task = task_list + 0;
	// should open fd 0 and 1
	init_task->parent = -1;
	init_task->status = TASK_ST_RUNNING;
	task_pid_allocator = 0;
}

int syscall_fork(int a, int b, int c) {
	int16_t pid, cur_pid;
	task_t *cur_task, *new_task;
	int i;

	pid = task_alloc_pid();
	if (pid < 0) {
		return pid;
	}
	cur_pid = task_current_pid();
	cur_task = task_list + cur_pid;
	new_task = task_list + new_pid;

	// Initialize task_t structure
	memcpy(new_task, cur_task, sizeof(task_t));
	new_task->pid = pid;
	new_task->parent = cur_pid;

	// Copy address space
	for (i = 0; i < TASK_MAX_PAGE_MAPS; i++) {
		if (!(new_task->pages[i].pt_flags & PAGE_DIR_ENT_PRESENT))
			break;
		if (new_task->pages[i].pt_flags & PAGE_DIR_ENT_RDWR) {
			// Writable page, need copy (mark as copy-on-write)
			new_task->pages[i].pt_flags &= ~PAGE_DIR_ENT_RDWR;
			new_task->pages[i].priv_flags |= TASK_PTENT_CPONWR;
		}
		// TODO: increase map count on physical page
	}

	// Return 0 to newly created process
	new_task->regs.eax = 0;

	// Done. New process will be executed by the scheduler later

	return new_pid;
}

int syscall_execve(int pathp, int argvp, int envpp) {
	char *path = (char *) pathp;
	char **argv = (char **) argvp;
	char **envp = (char **) envpp;
	int fd, ret;
	task_t *proc;

	// Sanity checks
	if (!pathp) {
		return -EFAULT;
	}

	proc = task_list + task_current_pid();

	// Permission checks
	fd = syscall_open(pathp, O_RDONLY, 0);
	if (fd < 0) {
		return fd;
	}
	ret = elf_load(fd);
	syscall_close(fd);

	if (ret != 0) {
		// Something very bad happened. Squash this process
		// TODO: jump to scheduler
	}

	// TODO: Push argv and envp onto stack, set ESP and EBP
	memset(proc->regs, 0, sizeof(proc->regs));

	// TODO: Initialize signal handlers

}

int syscall__exit(int status, int b, int c) {

}

int syscall_ece391_execute(int cmdlinep, int b, int c) {
	char *cmdline = (char *)cmdlinep;
}

int syscall_ece391_halt(int a, int b, int c) {
	return syscall__exit(0, 0, 0);
}
