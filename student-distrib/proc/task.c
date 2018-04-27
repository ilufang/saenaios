#include "task.h"

#include "../fs/vfs.h"
#include "elf.h"
#include "signal.h"
#include "scheduler.h"

task_t task_list[TASK_MAX_PROC];
pid_t task_pid_allocator;

typedef struct s_task_ks {
	int32_t pid;
	uint8_t stack[8188]; // Empty space to fill 8kb
} __attribute__((__packed__)) task_ks_t;

task_ks_t *kstack = (task_ks_t *)0x800000;

int16_t task_alloc_pid() {
	pid_t ret;
	for(ret = task_pid_allocator + 1; ret != task_pid_allocator; ret++) {
		if (ret >= TASK_MAX_PROC)
			ret = 0;
		if(task_list[ret].status == TASK_ST_NA) {
			task_pid_allocator = ret;
			return ret;
		}
	}
	// if there's no available pid left
	return -EAGAIN;
}

void task_create_kernel_pid() {
	int i;//iterator
	// initialize the kernel task
	task_t* init_task = task_list + 0;
	memset(init_task, 0, sizeof(task_t));
	// should open fd 0 and 1
	init_task->parent = -1;

	tss.ss0 = KERNEL_DS;
	tss.esp0 = init_task->ks_esp = (uint32_t)(kstack+1);
	task_pid_allocator = 0;

	// initialize kernel stack page
	for (i=0; i<512; ++i){
		kstack[i].pid = -1;
	}

	// kick start
	init_task->pid = 0;
	init_task->status = TASK_ST_RUNNING;
	kstack[0].pid = 0;

	// now iret to the kernel process
	scheduling_start();
	task_kernel_process_iret();
}

int syscall_getpid(int a, int b, int c) {
	return task_current_pid();
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
	new_task = task_list + pid;

	// Initialize task_t structure
	memcpy(new_task, cur_task, sizeof(task_t));
	new_task->pid = pid;
	new_task->parent = cur_pid;

	for (i = 0; i < TASK_MAX_OPEN_FILES; ++i) {
		if (new_task->files[i]) {
			new_task->files[i]->open_count++;
		}
	}

	// Create kernel stack (512 8kb entries in 4MB page)
	for (i = 0; i < 512; i++) {
		if (kstack[i].pid < 0) {
			// Slot is empty, use it
			kstack[i].pid = pid;
			new_task->ks_esp = (int)(kstack + i + 1);
			break;
		}
	}
	if (i == 512)
		return -ENOMEM;

	// Copy address space
	for (i = 0; i < TASK_MAX_PAGE_MAPS; i++) {
		if (!(new_task->pages[i].pt_flags & PAGE_DIR_ENT_PRESENT))
			break;
		if (new_task->pages[i].pt_flags & PAGE_DIR_ENT_RDWR) {
			// Writable page, need copy (mark as copy-on-write)
			new_task->pages[i].pt_flags &= ~PAGE_DIR_ENT_RDWR;
			new_task->pages[i].priv_flags |= TASK_PTENT_CPONWR;
			// Both have to be protected
			cur_task->pages[i].pt_flags &= ~PAGE_DIR_ENT_RDWR;
			cur_task->pages[i].priv_flags |= TASK_PTENT_CPONWR;

			if (cur_task->pages[i].pt_flags & PAGE_DIR_ENT_4MB) {
				// 4MB
				page_dir_delete_entry(cur_task->pages[i].vaddr);
				page_dir_add_4MB_entry(cur_task->pages[i].vaddr,
									   cur_task->pages[i].paddr,
									   cur_task->pages[i].pt_flags);
			} else {
				// 4KB
				page_tab_delete_entry(cur_task->pages[i].vaddr);
				page_tab_add_entry(cur_task->pages[i].vaddr,
								   cur_task->pages[i].paddr,
								   cur_task->pages[i].pt_flags);
			}
		}
		if (new_task->pages[i].pt_flags & PAGE_DIR_ENT_4MB) {
			// 4MB page
			page_alloc_4MB((int *)&(new_task->pages[i].paddr));
		} else {
			// 4KB page
			page_alloc_4KB((int *)&(new_task->pages[i].paddr));
		}
	}

	// Return 0 to newly created process
	new_task->regs.eax = 0;

	// Done. New process will be executed by the scheduler later

	page_flush_tlb();

	return pid; // Should not hit
}

int syscall_execve(int pathp, int argvp, int envpp) {
	char **argv = (char **) argvp;
	char **envp = (char **) envpp;
	int fd, ret, i;
	task_t *proc;
	uint32_t *u_argv, *u_envp, argc, envc;
	task_ptentry_t ptent_stack;

	// Sanity checks
	if (!pathp) {
		return -1;
	}

	proc = task_list + task_current_pid();

	// memset((char *)&(proc->regs), 0, sizeof(proc->regs));

	// TODO: Permission checks

	// Copy execution information to new user stack

	// allocate 4MB page for new stack (temporary at 0xc0000000 - 0xc0400000)
	ptent_stack.vaddr = 0xc0000000;
	ptent_stack.paddr = 0;
	ret = page_alloc_4MB((int *)&(ptent_stack.paddr));
	if (ret != 0) {
		// Page allocation failed. Probably ENOMEM
		return -1;
	}
	ptent_stack.priv_flags = 0;
	ptent_stack.pt_flags = PAGE_DIR_ENT_PRESENT;
	ptent_stack.pt_flags |= PAGE_DIR_ENT_RDWR;
	ptent_stack.pt_flags |= PAGE_DIR_ENT_USER;
	ptent_stack.pt_flags |= PAGE_DIR_ENT_4MB;
	page_dir_add_4MB_entry(ptent_stack.vaddr, ptent_stack.paddr,
						   ptent_stack.pt_flags);
	page_flush_tlb();

	// Push argv and envp onto stack, set ESP and EBP
	proc->regs.esp = 0xc0400000; // Default stack address

	task_user_pushl(&(proc->regs.esp), 0); // Reserve dword for args lookup

	// Parse argv
	u_argv = (uint32_t *) 0xc0000000; // Temporarily use top of stack as heap
	if (argv) {
		for (argc = 0; argv[argc]; argc++) {
			task_user_pushs(&(proc->regs.esp), (uint8_t *) argv[argc],
							strlen(argv[argc])+1);
			u_argv[argc] = proc->regs.esp - 0x400000;
		}
	} else {
		argc = 0;
	}
	u_argv[argc] = 0; // Terminating zero
	// Parse envp
	u_envp = u_argv + argc + 1;
	if (envp) {
		for (envc = 0; envp[envc]; envc++) {
			task_user_pushs(&(proc->regs.esp), (uint8_t *) envp[envc],
							strlen(envp[envc])+1);
			u_envp[envc] = proc->regs.esp - 0x400000;
		}
	} else {
		envc = 0;
	}
	u_envp[envc] = 0; // Terminating zero
	// Move temp values back
	task_user_pushs(&(proc->regs.esp), (uint8_t *) u_argv, 4*(argc+1));
	u_argv = (uint32_t *)(proc->regs.esp - 0x400000);
	task_user_pushs(&(proc->regs.esp), (uint8_t *) u_envp, 4*(envc+1));
	u_envp = (uint32_t *)(proc->regs.esp - 0x400000);
	task_user_pushl(&(proc->regs.esp), (uint32_t) u_envp);
	task_user_pushl(&(proc->regs.esp), (uint32_t) u_argv);
	task_user_pushl(&(proc->regs.esp), argc);

	// ece391_getargs workaround: bottom of stack points to argv
	*(uint32_t *)(0xc0400000 - 4) = (uint32_t) u_argv;

	strcpy((char *)0xc0000000, (char *)pathp); // Copy path to top-of-stack

	// Release previous process
	ret = task_current_pid();

	// Close all fd
	for (i = 2; i < TASK_MAX_OPEN_FILES; i++) {
		if (proc->files[i]) {
			syscall_close(i, 0, 0);
		}
	}

	scheduler_page_clear(proc->pages);
	task_release(proc);
	// Update kernel stack PID
	((task_ks_t *)(proc->ks_esp))[-1].pid = ret;

	// Re-map new stack
	page_dir_delete_entry(ptent_stack.vaddr);
	ptent_stack.vaddr = 0xbfc00000;
	page_dir_add_4MB_entry(ptent_stack.vaddr, ptent_stack.paddr,
						   ptent_stack.pt_flags);
	memcpy(proc->pages+0, &ptent_stack, sizeof(task_ptentry_t));
	proc->regs.esp -= 0x400000;
	page_flush_tlb();

	// Try to open ELF file for reading
	fd = syscall_open(0xbfc00000, O_RDONLY, 0);
	if (fd < 0) {
		page_alloc_free_4MB(ptent_stack.vaddr);
		syscall__exit(-1,0,0);
		return fd;
	}

	ret = elf_load(fd);
	syscall_close(fd, 0, 0);

	if (ret != 0) {
		// Something very bad happened. Squash this process
		syscall__exit(-1,0,0); // 1 for unsuccessful exit
		return -ENOEXEC; // This line should not hit though
	}

	// Initialize signal handlers
	for (i = 0; i < SIG_MAX; i++) {
		proc->sigacts[i].handler = SIG_DFL;
		proc->sigacts[i].flags = 0;
		sigemptyset(&(proc->sigacts[i].mask));
		sigaddset(&(proc->sigacts[i].mask), i);
	}

	proc->status = TASK_ST_RUNNING;

	page_flush_tlb();

	// set up tss
	tss.ss0 = KERNEL_DS;
	tss.esp0 = proc->ks_esp;

	// Jump to scheduler to execute
	scheduler_iret(&(proc->regs));

	return 0; // This line should not hit
}

int syscall__exit(int status, int b, int c) {
	task_t *proc, *parent;
	int i;

	proc = task_list + task_current_pid();
	parent = task_list + proc->parent;
	proc->regs.eax = status;

	// Close all fd
	for (i = 0; i < TASK_MAX_OPEN_FILES; i++) {
		if (proc->files[i]) {
			syscall_close(i, 0, 0);
		}
	}

	if (parent->status == TASK_ST_SLEEP || parent->status == TASK_ST_RUNNING) {
		// Parent is alive, notify parent
		proc->status = TASK_ST_ZOMBIE;
		syscall_kill(proc->parent, SIGCHLD, 0);
	} else {
		// Otherwise, just release the process
		scheduler_page_clear(proc->pages);
		task_release(proc);
	}

	scheduler_event();

	return 0; // This line should not hit
}

int syscall_wait(int statusp, int b, int c) {
	pid_t pid;
	int i, found_child;
	task_sigact_t sa;
	sigset_t ss;

	pid = task_current_pid();

	found_child = 0;
	for (i = 0; i < TASK_MAX_PROC; i++) {
		if (task_list[i].parent == pid) {
			switch (task_list[i].status) {
				case TASK_ST_ZOMBIE:
				if (statusp) {
					*(int *)statusp = task_list[i].regs.eax;
				}
				return task_list[i].pid;
				case TASK_ST_RUNNING:
				case TASK_ST_SLEEP:
					found_child = 1;
					break;
			}
		}
	}
	if (!found_child) {
		// No child process found
		return -ECHILD;
	}
	// Child process found, but non are terminated. Put parent to sleep
	sa.handler = SIG_IGN;
	sigemptyset(&(sa.mask));
	sa.flags = SA_RESTART;
	syscall_sigaction(SIGCHLD, (int)&sa, 0);
	sigemptyset(&ss);
	syscall_sigsuspend((int) &ss, NULL, 0);
	return 0; // Should not hit
}

int syscall_ece391_execute(int cmdlinep, int b, int c) {
	char *cmdline = (char *)cmdlinep;
	char *argv[2] = {NULL, NULL};
	int i;
	int child_pid;
	task_t* child_proc;
	sigset_t ss;
	uint32_t *kheap;

	if (!cmdlinep) {
		return -EFAULT;
	}
	while (*cmdline == ' ') cmdline++;
	argv[0] = cmdline;
	for (i = 0; cmdline[i]; i++) {
		if (!argv[1]) {
			// Space not detected yet
			if (cmdline[i] == ' ') {
				cmdline[i] = '\0';
				argv[1] = cmdline + i + 1;
			}
		} else {
			// Space detected
			if (cmdline[i] == ' ') {
				argv[1] = cmdline + i + 1;
			} else {
				break;
			}
		}
	}
	if (argv[1] && !*argv[1]) {
		argv[1] = NULL;
	}

	// -----fork a new process, go in, set up execute-----
	// no worries here since in the syscall, interrupts are all masked

	// directly call syscall_fork
	child_pid = syscall_fork(0,0,0);
	if (child_pid < 0){
		//error condition
		return -1;
	}
	child_proc = task_list + child_pid;

	// Put argv onto its kernel heap (top of kernel stack)
	kheap = (uint32_t *)(child_proc->ks_esp - sizeof(task_ks_t) + 4);
	kheap[1] = (uint32_t) argv[0];
	kheap[2] = (uint32_t) argv[1];
	kheap[3] = 0;

	// magically change user iret registers
	child_proc->regs.eax = SYSCALL_EXECVE;
	child_proc->regs.ebx = (uint32_t)cmdline;
	child_proc->regs.ecx = (uint32_t)(kheap + 1);
	child_proc->regs.edx = 0;
	child_proc->regs.eip = syscall_ece391_execute_magic + 0x8000000;

	// set parent to wait / sleep
	sigemptyset(&ss);
	syscall_sigsuspend((int) &ss, NULL, 0);

	return 0;
}

int syscall_ece391_halt(int a, int b, int c) {
	return syscall__exit(0, 0, 0);
}

int syscall_ece391_getargs(int bufp, int nbytes, int c) {
	char *buf;
	char** argv;
	int i, ptr, len;

	if (!bufp) {
		return -EFAULT;
	}

	buf = (char *) bufp;
	argv = *(char***)(0xc0000000 - 4);

	if (!argv[0] || !argv[1]) {
		return -1;
	}

	ptr = 0;
	for (i = 1; argv[i]; i++) {
		len = strlen(argv[i]);
		if (ptr + len + 1 >= nbytes) {
			// Not enough space
			return -1;
		}
		memcpy(buf + ptr, argv[i], len);
		ptr += len + 1;
		buf[ptr-1] = ' ';
	}
	buf[ptr-1] = '\0';
	strcpy((char *)buf, argv[1]);

	return 0;
}

void task_release(task_t *proc) {
	int i;
	// Mark program as dead
	proc->status = TASK_ST_DEAD;
	// Release all pages
	for (i = 0; i < TASK_MAX_PAGE_MAPS; i++) {
		if (!(proc->pages[i].pt_flags & PAGE_DIR_ENT_PRESENT)) {
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
	page_flush_tlb();
	// Release kernel stack
	((task_ks_t *)(proc->ks_esp))[-1].pid = -1;
	// Mark program as void
	proc->status = TASK_ST_NA;
}

int task_user_pushs(uint32_t *esp, uint8_t *buf, size_t size) {
	if (!buf) {
		return -EINVAL;
	}

	*esp -= size;
	*esp &= ~3; // Align to dword

	if (*esp < 0xbfc00000) {
		// Stack overflow!
		return -EFAULT;
	}

	memcpy((uint8_t *)(*esp), buf, size);

	return 0;
}

int task_user_pushl(uint32_t *esp, uint32_t val) {
	*esp -= 4;

	if (*esp < 0xbfc00000) {
		// Stack overflow!
		return -EFAULT;
	}

	*(uint32_t *)(*esp) = val;

	return 0;
}

int task_access_memory(uint32_t addr) {
	int i;
	task_t *proc;

	proc = task_list + task_current_pid();
	for (i = 0; i < TASK_MAX_PAGE_MAPS; i++) {
		if (addr < proc->pages[i].vaddr)
			continue;
		if (proc->pages[i].pt_flags & PAGE_DIR_ENT_4MB) {
			// 4MB page
			if (addr >= proc->pages[i].vaddr + (4<<20))
				continue;
		} else {
			// 4KB page
			if (addr >= proc->pages[i].vaddr + (4<<20))
				continue;
		}
		// In bounds, OK
		return 0;
	}

	return -EFAULT;
}

int task_pf_copy_on_write(uint32_t addr) {
	int i;
	task_t *proc;
	task_ptentry_t* page;

	proc = task_list + task_current_pid();
	for (i = 0; i < TASK_MAX_PAGE_MAPS; i++) {
		if (addr < proc->pages[i].vaddr)
			continue;
		if (proc->pages[i].pt_flags & PAGE_DIR_ENT_4MB) {
			// 4MB page
			if (addr >= proc->pages[i].vaddr + (4<<20))
				continue;
		} else {
			// 4KB page
			if (addr >= proc->pages[i].vaddr + (4<<20))
				continue;
		}
		// In bounds, check for copy-on-write flag
		if (!(proc->pages[i].priv_flags & TASK_PTENT_CPONWR))
			return -EFAULT;

		// OK. Copy page to be writable
		page = proc->pages + i;
		if (get_phys_mem_reference_count(page->paddr) == 1) {
			page->pt_flags |= PAGE_DIR_ENT_RDWR;
			page->priv_flags &= ~(TASK_PTENT_CPONWR);
			if (page->pt_flags & PAGE_DIR_ENT_4MB) {
				// 4MB page
				page_dir_delete_entry(page->vaddr);
				page_dir_add_4MB_entry(page->vaddr, page->paddr, page->pt_flags);
			} else {
				// 4KB page
				page_tab_delete_entry(page->vaddr);
				page_tab_add_entry(page->vaddr, page->paddr, page->pt_flags);
			}
			page_flush_tlb();
			return 0;
		}
		if (page->pt_flags & PAGE_DIR_ENT_4MB) {
			// 4MB page
			i = page->paddr;
			page->paddr = 0;
			page->pt_flags |= PAGE_DIR_ENT_RDWR;
			page->priv_flags &= ~(TASK_PTENT_CPONWR);
			// Allocate and copy memory (using virtual 0xc0000000 as temp)
			if (page_alloc_4MB((int *) &(page->paddr)) != 0) {
				// No memory... Delete this page
				page->pt_flags = 0;
				page_alloc_free_4MB(i);
				page_dir_delete_entry(page->vaddr);
				return -ENOMEM;
			}
			page_dir_add_4MB_entry(0xc0000000, page->paddr, page->pt_flags);
			memcpy((char *) 0xc0000000, (char *)page->vaddr, 4<<20);
			page_dir_delete_entry(0xc0000000);
			page_alloc_free_4MB(i);
			page_dir_delete_entry(page->vaddr);
			page_dir_add_4MB_entry(page->vaddr, page->paddr, page->pt_flags);
		} else {
			// 4KB page
			i = page->paddr;
			page->paddr = 0;
			page->pt_flags |= PAGE_DIR_ENT_RDWR;
			page->priv_flags &= ~(TASK_PTENT_CPONWR);
			// Allocate and copy memory (using virtual 0x08040000 as temp)
			if (page_alloc_4KB((int *) &(page->paddr)) != 0) {
				// No memory... Delete this page
				page->pt_flags = 0;
				page_alloc_free_4KB(i);
				page_tab_delete_entry(page->vaddr);
				return -ENOMEM;
			}
			page_tab_add_entry(0x08040000, page->paddr, page->pt_flags);
			memcpy((char *) 0x08040000, (char *) page->vaddr, 4<<10);
			page_tab_delete_entry(0x08040000);
			page_alloc_free_4KB(i);
			page_tab_delete_entry(page->vaddr);
			page_tab_add_entry(page->vaddr, page->paddr, page->pt_flags);
		}
		page_flush_tlb();
		return 0; // Resume program execution
	}
	return -EFAULT;
}

int task_make_initd(int a, int b, int c) {
	regs_t* regs;
	int ret;

	if (task_current_pid() != 0) {
		// Only initd can call syscall 15
		return -EPERM;
	}

	// load register address according to magic number on the stack
	regs = scheduler_get_magic();

	// save the registers of from process
	memcpy(&(task_list[0].regs), regs, sizeof(regs_t));

	ret = syscall_open((int)"/dev/stdin", O_RDONLY, 0);
	if (ret != 0) {
		printf("Cannot open stdin %d\n", ret);
	}
	ret = syscall_open((int)"/dev/stdout", O_WRONLY, 0);
	if (ret != 1) {
		printf("Cannot open stdout %d\n", ret);
	}
	return 0;
}
