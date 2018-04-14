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
		if (new_task->pages[i].pt_flags & PAGE_DIR_ENT_4MB) {
			// 4MB page
			page_alloc_4MB(&(new_task->pages[i].paddr));
		} else {
			// 4KB page
			page_alloc_4KB(&(new_task->pages[i].paddr));
		}
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
	int fd, ret, i;
	task_t *proc;
	uint32_t *u_argv, *u_envp, argc, envc;

	// Sanity checks
	if (!pathp) {
		return -EFAULT;
	}

	proc = task_list + task_current_pid();

	memset(proc->regs, 0, sizeof(proc->regs));

	// Permission checks
	fd = syscall_open(pathp, O_RDONLY, 0);
	if (fd < 0) {
		return fd;
	}
	ret = elf_load(fd);
	syscall_close(fd);

	if (ret != 0) {
		// Something very bad happened. Squash this process
		syscall__exit(1,0,0); // 1 for unsuccessful exit
		return -ENOEXEC; // This line should not hit though
	}

	// Push argv and envp onto stack, set ESP and EBP
	proc->regs.esp = 0xc0000000; // Default stack address
	proc->regs.ebp = 0xc0000000;

	task_user_pushl(&(proc->regs.esp), 0); // Reserve dword for args lookup

	// Parse argv
	u_argv = (uint32_t *) 0xbfc00000; // Temporarily use top of stack as heap
	if (argv) {
		for (argc = 0; argv[argc]; argc++) {
			task_user_pushs(&(proc->regs.esp), argv[argc], strlen(argv[argc]));
			u_argv[argc] = proc->regs.esp;
		}
	} else {
		argc = 0;
	}
	// Parse envp
	u_envp = u_argv + argc;
	if (envp) {
		for (envc = 0; envp[envc]; envc++) {
			task_user_pushs(&(proc->regs.esp), envp[envc], strlen(envp[envc]));
			u_envp[envc] = proc->regs.esp;
		}
		task_user_pushl(&(proc->regs.esp), 0); // Ending zero
	} else {
		envc = 0;
	}
	// Move temp values back
	task_user_pushs(&(proc->regs.esp), u_argv, 4*argc);
	u_argv = (uint32_t *) proc->regs.esp;
	task_user_pushs(&(proc->regs.esp), u_envp, 4*(envc+1));
	u_envp = (uint32_t *) proc->regs.esp;
	task_user_pushl(&(proc->regs.esp), (uint32_t) u_envp);
	task_user_pushl(&(proc->regs.esp), (uint32_t) u_argv);
	task_user_pushl(&(proc->regs.esp), argc);

	// ece391_getargs workaround: bottom of stack points to argv
	*(uint32_t)(0xc0000000 - 4) = (uint32_t) u_argv;

	// TODO: Initialize signal handlers


	// TODO: jump to scheduler to execute

	return 0; // This line should not hit
}

int syscall__exit(int status, int b, int c) {
	task_t *proc, *parent;

	proc = task_list + task_current_pid();
	parent = task_list + proc->parent;

	if (parent->status == TASK_ST_RUNNING) {
		// Parent is alive, notify parent
		proc->regs.eax = status;
		proc->status = TASK_ST_ZOMBIE;
		syscall_kill(proc->parent, SIGCHLD);
	} else {
		// Otherwise, just release the process
		task_release(proc);
	}

	// TODO: jump to scheduler

	return 0; // This line should not hit
}

int syscall_ece391_execute(int cmdlinep, int b, int c) {
	char *cmdline = (char *)cmdlinep;
}

int syscall_ece391_halt(int a, int b, int c) {
	return syscall__exit(0, 0, 0);
}

void task_release(task_t *proc) {
	int i;
	// Mark program as dead
	proc->status = TASK_ST_DEAD;
	// Close all fd
	for (i = 0; i < TASK_MAX_OPEN_FILES; i++) {
		if (proc->files[i]) {
			syscall_close(i);
		}
	}
	// Release all pages
	for (i = 0; i < TASK_MAX_PAGE_MAPS; i++) {
		if (proc->pages[i].pt_flags & PAGE_TAB_ENT_PRESENT) {
			// End of page list
			break;
		}
		if (proc->pages[i].pt_flags & PAGE_DIR_ENT_4MB) {
			// 4MB page
			page_alloc_free_4MB(proc->pages[i].paddr);
		} else {
			// 4KB page
			page_alloc_free_4KB(proc->pages[i].paddr);
		}
		proc->pages[i].pt_flags = 0;
	}
	// Mark program as void
	proc->status = TASK_ST_NA;
}

int task_user_pushs(uint32_t *esp, uint8_t *buf, size_t size) {
	if (!buf) {
		return -EINVAL;
	}

	*esp -= size;
	*esp &= ~3; // Align to dword

	if (esp < 0xbfc00000) {
		// Stack overflow!
		return -EFAULT;
	}

	memcpy((uint8_t *)(*esp), buf, size);

	return 0;
}

int task_user_pushl(uint32_t *esp, uint32_t val) {
	*esp -= 4;

	if (esp < 0xbfc00000) {
		// Stack overflow!
		return -EFAULT;
	}

	*(uint32_t *)(*esp) = val;

	return 0;
}
