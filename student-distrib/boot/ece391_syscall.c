#include "ece391_syscall.h"

#define ELF_MAGIC_SIZE	4

static uint8_t elf_signature[4] = {0x7f, 0x45, 0x4c, 0x46};		///< executable file signature

int32_t syscall_ece391_execute(const uint8_t* command){
	cli();
	// parse command & argument
	// trim front spaces
	uint8_t exec_command[FILENAME_LEN];
	memset(&exec_command, 0, FILENAME_LEN);
	int command_idx = 0;
	int i = 0;
	mp3fs_dentry_t exec_dentry;
	
	while(command[command_idx]==' '&&command[command_idx]!='\0'){
		command_idx++;
	}
	
	while(command[command_idx]!='\0'&&command[command_idx]!=' '&&command[command_idx]!='\n'){
		exec_command[i] = command[command_idx];
		i++;
		command_idx++;
		if(i>FILENAME_LEN)
			break;
	}
	// invalid command
	if(i <= 0){
		printf("Empty command\n");
		return -1;
	}
	// executable file does not exist
	if(read_dentry_by_name(exec_command, &exec_dentry)!=0){
		printf("File name does not exist\n");
		return -1;
	}
	uint8_t exec_magic_buf[ELF_MAGIC_SIZE];
	// read from file failed
	if(read_data(exec_dentry.inode_num, 0, exec_magic_buf, ELF_MAGIC_SIZE)!=ELF_MAGIC_SIZE){
		printf("Error reading file\n");
		return -1;
	}
	
	// file is not an executable
	if(strncmp((int8_t*)exec_magic_buf, (int8_t*)elf_signature, ELF_MAGIC_SIZE)!=0){
		printf("Trying to execute a non-executable file\n");
		return -1;
	}

	int32_t new_pid = get_new_pid();
	// no more available pid
	if(new_pid < 0){
		printf("No more available pid\n");
		return -1;
	}
	
	// TODO: setup program paging
	page_dir_add_4MB_entry(128 * M_BYTE, PHYS_MEM_OFFSET + (new_pid - 1) * 4 * M_BYTE, PAGE_DIR_ENT_PRESENT | PAGE_DIR_ENT_RDWR | PAGE_DIR_ENT_USER);
	page_flush_tlb();
	
	uint8_t exec_entry[4];
	// read entry point failed
	if(read_data(exec_dentry.inode_num, 24, (uint8_t*)exec_entry, ELF_MAGIC_SIZE)!=ELF_MAGIC_SIZE){
		printf("Error reading entry point\n");
		return -1;
	}
	int32_t entry_addr = 0;
	// convert the 4 bytes into an 32 bit address
	entry_addr |= exec_entry[3]<<24;
	entry_addr |= exec_entry[2]<<16;
	entry_addr |= exec_entry[1]<<8;
	entry_addr |= exec_entry[0];
	
	// uint8_t content[M_BYTE];
	// load program & check for failure
	if(read_data(exec_dentry.inode_num, 0, (uint8_t*)PROG_IMG_OFFSET, M_BYTE)<0){
		printf("Error loading program\n");
		return -1;
	}
	
	pcb_t* curr_pcb = get_curr_pcb();
	
	// save registers in parent pcb
	asm volatile (
			"								\n\
			movl	%%esp, %%eax			\n\
			movl	%%ebp, %%ebx			\n\
			pushfl							\n\
			popl	%%ecx					\n\
			"
            : "=a"(curr_pcb->esp), "=b"(curr_pcb->ebp), "=c"(curr_pcb->flags)
            : /* no inputs */
    );
	
	(curr_pcb->flags)|=0x200;
	
	pcb_t* new_pcb = get_pcb_addr(new_pid);
	new_pcb->curr_pid = new_pid;
	new_pcb->parent_pid = curr_pcb->curr_pid;
	new_pcb->state = PROC_STATE_RUNNING;
	// hard code fd 0 and 1???
	curr_pcb->state = PROC_STATE_WAITING;
	proc_list[new_pid] = new_pcb;
	if(set_active_pcb(new_pid)==-1){
		return -1;
	}
	
	int32_t fd_in, fd_out;
	fd_in = open("/dev/stdin", O_RDONLY, 0);
	fd_out = open("/dev/stdout", O_WRONLY, 0);
	
	// modify tss
	tss.ss0 = KERNEL_DS;
	// set tss.esp0 to point to the new kernel stack
	tss.esp0 = 8 * M_BYTE - (new_pid) * 8 * K_BYTE - 4;
	int usr_stack = (128 + 4) * M_BYTE - 4; 
	
	sti();
	
	// context switch
	asm volatile (
			"								\n\
			movl	%2, %%eax				\n\
			movw	%%ax, %%ds				\n\
			pushl	%%eax					\n\
			pushl	%3						\n\
			pushfl							\n\
			pushl	%1						\n\
			pushl	%0						\n\
			iret							\n\
			EXEC_RETURN:					\n\
			leave							\n\
			ret								\n\
			"
			: 
			: "r"(entry_addr), "r"(USER_CS), "r"(USER_DS), "r"(usr_stack)
			: "%eax"
	);
	
	return 0;
	
}

int32_t syscall_ece391_halt(uint8_t status){
	
	pcb_t* curr_pcb = get_curr_pcb();
	pcb_t* parent_pcb = get_pcb_addr(curr_pcb->parent_pid);
	int parent_addr = PHYS_MEM_OFFSET + (curr_pcb->parent_pid - 1) * 4 * M_BYTE;
	
	// TODO: restore parent paging
	page_dir_add_4MB_entry(128 * M_BYTE, parent_addr, PAGE_DIR_ENT_PRESENT | PAGE_DIR_ENT_RDWR | PAGE_DIR_ENT_USER);
	page_flush_tlb();
	
	// TODO: restore fds
	int i = 0;
	
	for(i = 0; i < PCB_MAX_FILE; i++){
		if((curr_pcb->fd_arr[i])!=NULL)
			close(i);
	}
	
	
	// set tss.esp0 to point to parent kernel stack
	tss.esp0 = parent_pcb->esp;
	
	if(set_active_pcb(curr_pcb->parent_pid)==-1){
		return -1;
	}
	proc_list[curr_pcb->curr_pid] = NULL;
	parent_pcb->state = PROC_STATE_RUNNING;
	
	// retore parent process and return to execute return
	asm volatile (
			"								\n\
			movl	%0, %%esp				\n\
			movl	%1, %%ebp				\n\
			xorl	%%eax, %%eax			\n\
			movb	%3, %%al				\n\
			pushl	%2						\n\
			popfl							\n\
			jmp		EXEC_RETURN				\n\
			"
            : 
            : "r"(parent_pcb->esp), "r"(parent_pcb->ebp), "r"(parent_pcb->flags), "r"(status)
			: "%eax"
	);
	
	// just to avoid warnings
	return 0;
}


