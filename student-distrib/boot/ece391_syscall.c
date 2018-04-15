#include "ece391_syscall.h"

#define ELF_MAGIC_SIZE	4

static uint8_t elf_signature[4] = {0x7f, 0x45, 0x4c, 0x46};		///< executable file signature

static page_table_t ece391_usr_page_table;

int32_t syscall_ece391_execute(int command_addr, int b, int c){
	char* command = (char*)command_addr;
	cli();
	// parse command & argument
	// trim front spaces
	uint8_t exec_command[FILENAME_LEN];
	memset(&exec_command, 0, FILENAME_LEN);
	int command_idx = 0;
	int i = 0;
	mp3fs_dentry_t exec_dentry;
	
	// invalid parameter
	if(!command){
		return -1;
	}
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
		//printf("Empty command\n");
		return -1;
	}
	
	int32_t new_pid = get_new_pid();
	// no more available pid
	if(new_pid < 0){
		//printf("No more available pid\n");
		return -1;
	}
	task_t* new_task = get_task_addr(new_pid);
	i = 0;
	while(command[command_idx]==' '){
		command_idx++;
	}
	memset(&(new_task->args), 0, MAX_ARGS);
	
	while(command[command_idx]!='\0'&&command[command_idx]!='\n'){
		new_task->args[i] = command[command_idx];
		i++;
		command_idx++;
		if(i>MAX_ARGS)
			break;
	}
	
	// executable file does not exist
	if(read_dentry_by_name(exec_command, &exec_dentry)!=0){
		//printf("File name does not exist\n");
		return -1;
	}
	uint8_t exec_magic_buf[ELF_MAGIC_SIZE];
	// read from file failed
	if(read_data(exec_dentry.inode_num, 0, exec_magic_buf, ELF_MAGIC_SIZE)!=ELF_MAGIC_SIZE){
		//printf("Error reading file\n");
		return -1;
	}
	
	// file is not an executable
	if(strncmp((int8_t*)exec_magic_buf, (int8_t*)elf_signature, ELF_MAGIC_SIZE)!=0){
		//printf("Trying to execute a non-executable file\n");
		return -1;
	}

	
	
	// TODO: setup program paging
	page_dir_add_4MB_entry(128 * M_BYTE, PHYS_MEM_OFFSET + (new_pid) * 4 * M_BYTE, PAGE_DIR_ENT_PRESENT | PAGE_DIR_ENT_RDWR | PAGE_DIR_ENT_USER);
	page_flush_tlb();
	
	uint8_t exec_entry[4];
	// read entry point failed
	if(read_data(exec_dentry.inode_num, ENTRY_POINT_OFFSET, (uint8_t*)exec_entry, ELF_MAGIC_SIZE)!=ELF_MAGIC_SIZE){
		//printf("Error reading entry point\n");
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
	if(read_data(exec_dentry.inode_num, 0, (uint8_t*)PROG_IMG_OFFSET, 4 * M_BYTE)<0){
		//printf("Error loading program\n");
		return -1;
	}
	
	task_t* curr_task = get_curr_task();
	
	// save registers in parent task
	asm volatile (
			"								\n\
			movl	%%esp, %%eax			\n\
			movl	%%ebp, %%ebx			\n\
			pushfl							\n\
			popl	%%ecx					\n\
			"
            : "=a"(curr_task->esp), "=b"(curr_task->ebp), "=c"(curr_task->flags)
            : /* no inputs */
    );
	
	(curr_task->flags)|=0x200;
	
	new_task->curr_pid = new_pid;
	new_task->parent_pid = curr_task->curr_pid;
	new_task->status = TASK_ST_RUNNING;
	new_task->vid_mapped = 0;
	// hard code fd 0 and 1???
	curr_task->status = TASK_ST_NA;
	new_task->files[0] = curr_task->files[0];
	new_task->files[1] = curr_task->files[1];
	for (i=2;i<TASK_MAX_OPEN_FILES;++i){
		new_task->files[i] = NULL;
	}
 	task_list[new_pid] = new_task;
	
	
	
	
	// modify tss
	tss.ss0 = KERNEL_DS;
	// set tss.esp0 to point to the new kernel stack
	tss.esp0 = 8 * M_BYTE - (new_pid) * 8 * K_BYTE - 4;
	int usr_stack = (128 + 4) * M_BYTE - 4; 
	
	sti();
	
	// context switch
	asm volatile (
			"								\n\
			cli								\n\
			movl	%2, %%eax				\n\
			movw	%%ax, %%ds				\n\
			pushl	%%eax					\n\
			pushl	%3						\n\
			pushfl							\n\
			popl	%%eax					\n\
			orl		$0x200, %%eax			\n\
			pushl	%%eax					\n\
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

int32_t syscall_ece391_halt(int status_in, int b, int c){
	// take the lower 8 bits
	uint8_t status = status_in & 0xFF;	
	cli();
	// get current task
	task_t* curr_task = get_curr_task();
	task_t* parent_task = get_task_addr(curr_task->parent_pid);
	// calculate parent address
	int parent_addr = PHYS_MEM_OFFSET + (curr_task->parent_pid) * 4 * M_BYTE;
	
	// restore parent paging
	page_dir_add_4MB_entry(128 * M_BYTE, parent_addr, PAGE_DIR_ENT_PRESENT | PAGE_DIR_ENT_RDWR | PAGE_DIR_ENT_USER);
	
	// turn off video mapping if parent task is unmapped
	
	if(parent_task->vid_mapped == 0){
		page_dir_add_4KB_entry(VID_VIRT_OFFSET, (void*)(&ece391_usr_page_table), PAGE_DIR_ENT_RDWR | PAGE_DIR_ENT_USER);
		page_tab_add_entry(VID_VIRT_OFFSET, VID_MEM_OFFSET, PAGE_TAB_ENT_RDWR | PAGE_TAB_ENT_USER);
	}
	page_flush_tlb();
	
	// close fds of current task
	int i = 0;
	
	for(i = 2; i < TASK_MAX_OPEN_FILES; i++){
		if((curr_task->files[i])!=NULL)
			close(i);
	}
	
	memset(&(curr_task->args), 0, MAX_ARGS);
	
	// set tss.esp0 to point to parent kernel stack
	tss.esp0 = 8 * M_BYTE - (parent_task->curr_pid) * 8 * K_BYTE - 4;
	
	task_list[curr_task->curr_pid] = NULL;
	parent_task->status = TASK_ST_RUNNING;
	
	sti();
	
	// check if its the first shell
	if(parent_task->curr_pid == 0){
		asm volatile (
			"								\n\
			movl	%0, %%esp				\n\
			movl	%0, %%ebp				\n\
			"
            : 
            : "r"(parent_task->ebp + 8)
		);
		syscall_ece391_execute((int)"shell",0,0);
	}
	// retore parent process and return to execute return
	asm volatile (
			"								\n\
			movl	%0, %%esp				\n\
			movl	%1, %%ebp				\n\
			xorl	%%eax, %%eax			\n\
			movb	%2, %%al				\n\
			jmp		EXEC_RETURN				\n\
			"
            : 
            : "r"(parent_task->esp), "r"(parent_task->ebp), "r"(status)
			: "%eax"
	);
	
	// just to avoid warnings
	return 0;
}


int32_t syscall_ece391_getargs(int buf_in, int nbytes, int c){
	// input wrap
	uint8_t* buf = (uint8_t*)buf_in;

	// sanity check
	if(!buf){
		return -1;
	}
	// clear buffer in case of leftover from previous operations
	memset(buf, 0, nbytes);
	int i = 0;
	// get current task
	task_t* curr_task = get_curr_task();
	// return -1 for empty argument
	if(curr_task->args[0]=='\0'){
		return -1;
	}
	// copy arguments into buffer
	while(curr_task->args[i]!='\0' && i < nbytes){
		// check whether it fits
		if(i >= nbytes - 1 && curr_task->args[i]!='\0')
			return -1;
		
		buf[i] = curr_task->args[i];
		i++;
		// get to end of args buffer in pcb
		if(i >= MAX_ARGS){
			*(buf + i) = '\0';
			break;
		}
	}
	return 0;
	
}

int32_t syscall_ece391_vidmap(int screen_start_in, int b, int c){
	uint8_t** screen_start = (uint8_t **)screen_start_in;
	// check if passed in a valid pointer
	if(!screen_start ||(int32_t)screen_start < 128 * M_BYTE || (int32_t)screen_start >= 132 * M_BYTE){
		return -1;
	}
	task_t* curr_task = get_curr_task();
	// set video map flag for current task
	curr_task->vid_mapped = 1;
	// map the arbitrary virtual address to video memory physical address
	page_dir_add_4KB_entry(VID_VIRT_OFFSET, (void*)(&ece391_usr_page_table), PAGE_DIR_ENT_PRESENT | PAGE_DIR_ENT_RDWR | PAGE_DIR_ENT_USER);
	page_tab_add_entry(VID_VIRT_OFFSET, VID_MEM_OFFSET, PAGE_TAB_ENT_PRESENT | PAGE_TAB_ENT_RDWR | PAGE_TAB_ENT_USER);
	page_flush_tlb();
	// store the address in pointer passed in
	*screen_start = (uint8_t*)VID_VIRT_OFFSET;
	return VID_VIRT_OFFSET;
}

