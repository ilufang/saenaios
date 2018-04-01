#include "ece391_syscall.h"

#define ELF_MAGIC_SIZE	4

static uint8_t elf_signature[4] = {0x7f, 0x45, 0x4c, 0x46};

int32_t syscall_ece391_execute(const uint8_t* command){
	
	// parse command & argument
	// trim front spaces
	uint8_t exec_command[FILENAME_LEN];
	int command_idx = 0;
	int i = 0;
	fsys_dentry_t exec_dentry;
	
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
		return -1;
	}
	// executable file does not exist
	if(read_dentry_by_name(exec_command, &exec_dentry)!=0){
		return -1;
	}
	uint8_t exec_magic_buf[ELF_MAGIC_SIZE];
	// read from file failed
	if(read_data(exec_dentry.inode_num, 0, exec_magic_buf, ELF_MAGIC_SIZE)!=4){
		return -1;
	}
	
	// file is not an executable
	if(exec_magic_buf[0]!=elf_signature[0]||exec_magic_buf[1]!=elf_signature[1]||
	  exec_magic_buf[2]!=elf_signature[2]||exec_magic_buf[3]!=elf_signature[3]){
		return -1;
	}

	int32_t new_pid = get_new_pid();
	// no more available pid
	if(new_pid < 0){
		return -1;
	}
	
	// TODO: setup program paging
	page_table_add_entry(128 * M_BYTE, (8 + new_pid) * M_BYTE, PAGE_TAB_ENT_USR);
	page_flush_tlb();
	
	int32_t exec_entry;
	// read entry point failed
	if(read_data(exec_dentry.inode_num, 24, (uint8_t*)exec_entry, ELF_MAGIC_SIZE)!=4){
		return -1;
	}
	// load program & check for failure
	if(read_data(exec_dentry.inode_num, 0, (uint8_t*)PROG_IMG_OFFSET, M_BYTE)<0){
		return -1;
	}
	
	// TODO: setup pcb
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
	
	
	pcb_t* new_pcb = get_pcb_addr(new_pid);
	new_pcb->curr_pid = new_pid;
	new_pcb->parent_pid = curr_pcb->curr_pid;
	new_pcb->state = PROC_STATE_RUNNING;
	curr_pcb->state = PROC_STATE_WAITING;
	
	// modify tss
	tss.ss0 = KERNEL_DS;
	tss.esp0 = 8 * M_BYTE - (new_pid) * 8 * K_BYTE - 4;
	
	// context switch (how to get stack segment?)
	asm volatile (
			"								\n\
			movl	$0x2B, %%eax			\n\
			movw	%%ax, %%ds				\n\
            pushl	$0x2B					\n\
            pushl	$0x83FFFFC				\n\
			pushfl							\n\
            pushl	$0x28					\n\
			pushl	%0						\n\
			iret							\n\
			IRET_RETURN:					\n\
			"
            : 
            : "r"(exec_entry)
			: "%eax"
	);
	
	
}

int32_t halt(uint8_t status){
	
	pcb_t* curr_pcb = get_curr_pcb();
	pcb_t* parent_pcb = get_pcb_addr(curr_pcb->parent_pid);
	
	// TODO: restore parent paging
	page_table_add_entry(128 * M_BYTE, (8 + parent_pcb->pid) * M_BYTE, PAGE_TAB_ENT_USR);
	page_flush_tlb();
	
	// TODO: restore fds
	int i = 0;
	for(i = 0; i < PCB_MAX_FILE; i++){
		if(curr_pcb->fd_arr[i])
			close(i);
	}
	
	proc_list[curr_pcb->curr_pid] = NULL;
	parent_pcb->state = PROC_STATE_RUNNING;
	
	asm volatile (
			"								\n\
			movl	%0, %%esp				\n\
			movl	%1, %%ebp				\n\
			xorl	%%eax, %%eax			\n\
			movb	%%bl, %%al				\n\
            pushl	%2						\n\
            popfl							\n\
			jmp IRET_RETURN					\n\
			"
            : 
            : "r"(parent_pcb->esp), "r"(parent_pcb->ebp), "r"(parent_pcb->flags)
			: "%eax"
	);
}


