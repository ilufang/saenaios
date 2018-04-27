#include "tty.h"

static tty_t tty_list[TTY_NUMBER];

#define VIDMEM_START 	0xB8000
#define TTY_4KB 		0x1000

static int vidmem_index = 0;

void tty_start(){
	int i; //iterator
	// initialize all terminals to accept keyboard input
	for (i=0; i<TTY_NUMBER; ++i){
		tty_status = TTY_SLEEP;
	}

	// activate the first tty for the start up terminal
	tty_terminal_init(&tty_list[0])
	tty_list[0].tty_status = TTY_FOREGROUND;
}

int tty_terminal_init(tty_t* tty){
	// allocate the video memory first
	tty->vidmem.vaddr = VIDMEM_START + TTY_4KB * vidmem_index;
	tty->vidmem.paddr = tty->vidmem.vaddr;
	tty->vidmem.pt_flags = PAGE_TAB_ENT_PRESENT | PAGE_TAB_ENT_RDWR | PAGE_TAB_ENT_SUPERVISOR | PAGE_TAB_ENT_GLOBAL;

	if (!page_tab_add_entry(tty->vidmem.vaddr, tty->vidmem.paddr, tty->vidmem.pt_flags)){
		return -EFAULT;
	}


	vidmem_index ++;
}

int _tty_switch(tty_t* from, tty_t* to){
	// switch address space

	// change tty structure status
}

int _get_current_tty(){
	int i;

	for (i=0; i<TTY_NUMBER; ++i){
		if (tty_list[i].tty_status & TTY_FOREGROUND){
			// found the foreground tty
			return i;
		}
	}
	return -1;	// very bad thing happened
}


