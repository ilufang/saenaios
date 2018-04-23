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
	task_kernel_process_iret();
}

int syscall_fork(int a, int b, int c) {
	cli();
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
		return -EFAULT;
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
		return ret;
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
			task_user_pushs(&(proc->regs.esp), (uint8_t *) argv[argc], strlen(argv[argc]));
			u_argv[argc] = proc->regs.esp;
		}
	} else {
		argc = 0;
	}
	// Parse envp
	u_envp = u_argv + argc;
	if (envp) {
		for (envc = 0; envp[envc]; envc++) {
			task_user_pushs(&(proc->regs.esp), (uint8_t *) envp[envc], strlen(envp[envc]));
			u_envp[envc] = proc->regs.esp;
		}
		task_user_pushl(&(proc->regs.esp), 0); // Ending zero
	} else {
		envc = 0;
	}
	// Move temp values back
	task_user_pushs(&(proc->regs.esp), (uint8_t *) u_argv, 4*argc);
	u_argv = (uint32_t *) proc->regs.esp;
	task_user_pushs(&(proc->regs.esp), (uint8_t *) u_envp, 4*(envc+1));
	u_envp = (uint32_t *) proc->regs.esp;
	task_user_pushl(&(proc->regs.esp), (uint32_t) u_envp);
	task_user_pushl(&(proc->regs.esp), (uint32_t) u_argv);
	task_user_pushl(&(proc->regs.esp), argc);

	// ece391_getargs workaround: bottom of stack points to argv
	*(uint32_t *)(0xc0400000 - 4) = (uint32_t) u_argv;

	strcpy((char *)0xc0000000, (char *)pathp); // Copy path to top-of-stack

	// Release previous process
	ret = task_current_pid();
	task_release(proc);
	// Update kernel stack PID
	((task_ks_t *)(proc->ks_esp))[-1].pid = ret;


	// Re-map new stack
	page_dir_delete_entry(ptent_stack.vaddr);
	ptent_stack.vaddr = 0xbfc00000;
	page_dir_add_4MB_entry(ptent_stack.vaddr, ptent_stack.paddr,
						   ptent_stack.pt_flags);
	memcpy(proc->pages+0, &ptent_stack, sizeof(task_ptentry_t));
	proc->regs.esp = 0xc0000000;
	page_flush_tlb();


	// Try to open ELF file for reading
	fd = syscall_open(0xbfc00000, O_RDONLY, 0);
	if (fd < 0) {
		page_alloc_free_4MB(ptent_stack.vaddr);
		return fd;
	}

	ret = elf_load(fd);
	syscall_close(fd, 0, 0);

	if (ret != 0) {
		// Something very bad happened. Squash this process
		syscall__exit(1,0,0); // 1 for unsuccessful exit
		return -ENOEXEC; // This line should not hit though
	}

	// Initialize FD 0, 1
	ret = syscall_open((int)"/dev/stdin", O_RDONLY, 0);
	if (ret != 0) {
		printf("Failed to open stdin\n");
	}
	ret = syscall_open((int)"/dev/stdout", O_WRONLY, 0);
	if (ret != 1) {
		printf("Failed to open stdout\n");
	}

	// Initialize signal handlers
	for (i = 0; i < SIG_MAX; i++) {
		proc->sigacts[i].handler = SIG_DFL;
		proc->sigacts[i].flags = 0;
		sigfillset(proc->sigacts[i].mask);
		sigdelset(proc->sigacts[i].mask, i);
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

	proc = task_list + task_current_pid();
	parent = task_list + proc->parent;

	if (parent->status == TASK_ST_RUNNING) {
		// Parent is alive, notify parent
		proc->regs.eax = status;
		proc->status = TASK_ST_ZOMBIE;
		// syscall_kill(proc->parent, SIGCHLD); // TODO
	} else {
		// Otherwise, just release the process
		task_release(proc);
	}

	scheduler_event();

	return 0; // This line should not hit
}

int syscall_ece391_execute(int cmdlinep, int b, int c) {
	char *cmdline = (char *)cmdlinep;
	char *argv[2] = {NULL, NULL};
	int i;
	int child_pid;
	task_t* child_proc;
	task_t* parent_proc = task_list + task_current_pid();
	sigset_t ss;

	if (!cmdlinep) {
		return -EFAULT;
	}
	while (*cmdline == ' ') cmdline++;
	for (i = 0; cmdline[i]; i++) {
		if (!argv) {
			// Space not detected yet
			if (cmdline[i] == ' ') {
				cmdline[i] = '\0';
				argv[0] = cmdline + i + 1;
			}
		} else {
			// Space detected
			if (cmdline[i] == ' ') {
				argv[0] = cmdline + i + 1;
			} else {
				break;
			}
		}
	}

	// -----fork a new process, go in, set up execute-----
	// no worries here since in the syscall, interrupts are all masked

	// directly call syscall_fork
	child_pid = syscall_fork(0,0,0);
	if (child_pid<0){
		//error condition
		return child_pid;
	}
	child_proc = task_list + child_pid;

	// need to update parent process' regs so next time parent process could correctly return
	memcpy(&(parent_proc->regs), scheduler_get_magic(), sizeof(regs_t));

	// magically change user iret registers
	child_proc->regs.eax = SYSCALL_EXECVE;
	child_proc->regs.ebx = (int)cmdline;
	child_proc->regs.ecx = (int)argv;
	child_proc->regs.edx = 0;
	child_proc->regs.eip = syscall_ece391_execute_magic + 0x8000000;

	// set parent to wait / sleep
	sigfillset(ss);
	sigdelset(ss, SIGIO);
	syscall_sigsuspend((int) &ss, NULL, 0);

	return 0;
}

int syscall_ece391_halt(int a, int b, int c) {
	return syscall__exit(0, 0, 0);
}

int syscall_ece391_getargs(int buf, int nbytes, int c) {
	return -ENOSYS; // TODO
}

void task_release(task_t *proc) {
	int i;
	// Mark program as dead
	proc->status = TASK_ST_DEAD;
	// Close all fd
	for (i = 0; i < TASK_MAX_OPEN_FILES; i++) {
		if (proc->files[i]) {
			syscall_close(i, 0, 0);
		}
	}
	// Release all pages
	scheduler_page_clear(proc->pages);
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

	if (task_current_pid() != 0) {
		// Only initd can call syscall 15
		return -EPERM;
	}

	// load register address according to magic number on the stack
	regs = scheduler_get_magic();

	// save the registers of from process
	memcpy(&(task_list[0].regs), regs, sizeof(regs_t));

	return 0;
}
