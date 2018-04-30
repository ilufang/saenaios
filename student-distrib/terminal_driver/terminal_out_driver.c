#include "terminal_out_driver.h"

#define VIDMEM_START 	0xB8000
#define TTY_4KB 		0x1000

static stdout_data_t stdout[MAX_STDOUT];

static int stdout_index = 0;

static file_operations_t terminal_out_op;

static uint8_t* video_mem = (uint8_t*)VIDMEM_START;

static int lscreen_x = 0, lscreen_y = 0;

static int lcursor_x = 0, lcursor_y = 0;

static int last_newline;

// macro used to write a word to a port
#define OUTW(port, val)                                             \
do {                                                                \
    asm volatile ("outw %w1, (%w0)"                                 \
        : /* no outputs */                                          \
        : "d"((port)), "a"((val))                                   \
        : "memory", "cc"                                            \
    );                                                              \
} while (0)
/* macro used to write a byte to a port */
#define OUTB(port, val)                                             \
do {                                                                \
    asm volatile ("outb %b1, (%w0)"                                 \
        : /* no outputs */                                          \
        : "d"((port)), "a"((val))                                   \
        : "memory", "cc"                                            \
    );                                                              \
} while (0)

int terminal_out_driver_register(){
	// inflate the operation pointer table
	terminal_out_op.open = & terminal_out_open;
	terminal_out_op.release = & terminal_out_close;
	terminal_out_op.read = NULL;
	terminal_out_op.write = & terminal_out_write;
	terminal_out_op.readdir = NULL;

	return (devfs_register_driver("stdout", &terminal_out_op));
}

int terminal_out_open(inode_t* inode, file_t* file){
	//no private data to be set for now
	terminal_set_cursor();
	//cannot return to last line
	last_newline = 1;

	return 0;
}

int terminal_out_close(inode_t* inode,file_t* file){
	return 0; // does nothing for now
}

void* terminal_out_tty_init(){
	if (stdout_index >= MAX_STDOUT){
		errno = -ENOSPC;
		return NULL;
	}

	// initialize a new stdout private data
	stdout[stdout_index].screen_x = 0;
	stdout[stdout_index].screen_y = 0;
	stdout[stdout_index].cursor_x = 0;
	stdout[stdout_index].cursor_y = 0;
	stdout[stdout_index].newline = 1;

	// allocate the video memory
	stdout[stdout_index].vidmem.vaddr = VIDMEM_START + TTY_4KB * stdout_index;
	stdout[stdout_index].vidmem.paddr = stdout[stdout_index].vidmem.vaddr;
	stdout[stdout_index].vidmem.pt_flags = PAGE_TAB_ENT_PRESENT | PAGE_TAB_ENT_RDWR | PAGE_TAB_ENT_SUPERVISOR | PAGE_TAB_ENT_GLOBAL;

	if (_page_tab_add_entry(stdout[stdout_index].vidmem.vaddr,
		stdout[stdout_index].vidmem.paddr, stdout[stdout_index].vidmem.pt_flags)){
		errno = EFAULT;
		return NULL;
	}

	video_mem = (uint8_t*)stdout[stdout_index].vidmem.vaddr;
	terminal_out_clear();

	stdout_index++;

	return (void*)(&stdout[stdout_index-1]);
}

int terminal_out_tty_switch(void* fromp, void* top){
	stdout_data_t* from = (stdout_data_t*)fromp;
	stdout_data_t* to = (stdout_data_t*)top;
	int i, ret;
	uint32_t temp;
	// switch physical memory
	for (i=0;i<NUM_COLS*NUM_ROWS/2;++i){
		temp = *((uint32_t*)from->vidmem.vaddr + i);
		*((uint32_t*)from->vidmem.vaddr + i) = *((uint32_t*)to->vidmem.vaddr + i);
		*((uint32_t*)to->vidmem.vaddr + i) = temp;
	}
	// switch page entries virtual memory mapping
	_page_tab_delete_entry(from->vidmem.vaddr);
	_page_tab_delete_entry(to->vidmem.vaddr);
	temp = from->vidmem.paddr;
	from->vidmem.paddr = to->vidmem.paddr;
	to->vidmem.paddr = temp;
	ret = _page_tab_add_entry(from->vidmem.vaddr,from->vidmem.paddr,from->vidmem.pt_flags);
	if (ret) return ret;
	ret = _page_tab_add_entry(to->vidmem.vaddr,to->vidmem.paddr,to->vidmem.pt_flags);
	if (ret) return ret;

	// update cursor
	lscreen_x = to->screen_x;
	lscreen_y = to->screen_y;
	terminal_set_cursor();
	return 0;
}

void terminal_set_cursor(){
	// calculate memory position for cursor position
	int pos = lscreen_y * NUM_COLS + lscreen_x;
 	// asm operation for setting cursor position
	outb(0x0F,0x3D4);
	outb((uint8_t) (pos & 0xFF), 0x3D5);
	outb(0x0E, 0x3D4);
	outb((uint8_t) ((pos >> 8) & 0xFF), 0x3D5);

}

void terminal_out_clear(){
	// clear out memory
	int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
        *(uint8_t *)(video_mem + (i << 1)) = ' ';
        *(uint8_t *)(video_mem + (i << 1) + 1) = ATTRIB;
    }
	// set cursor to up left
	lscreen_y = 0;
	lscreen_x = 0;
	lcursor_y = 0;
	lcursor_x = 0;
	last_newline = 1;
}

ssize_t terminal_out_write(file_t* file, uint8_t* buf,size_t count,off_t* offset){
	return (ssize_t)(terminal_out_write_(buf,(int)count));	// wrap for now
}

ssize_t tty_stdout(uint8_t* data, uint32_t size, void* private_data_ptr){
	stdout_data_t* private_data = (stdout_data_t*)private_data_ptr;
	ssize_t ret_value;
	// get the private data to calibrate to the tty needed
	lcursor_x = private_data -> cursor_x;
	lcursor_y = private_data -> cursor_y;
	lscreen_x = private_data -> screen_x;
	lscreen_y = private_data -> screen_y;
	video_mem = (uint8_t*)private_data -> vidmem.vaddr;
	last_newline = private_data -> newline;
	// write
	ret_value = terminal_out_write_(data, size);

	// update back the private data
	private_data -> cursor_x = lcursor_x;
	private_data -> cursor_y = lcursor_y;
	private_data -> screen_x = lscreen_x;
	private_data -> screen_y = lscreen_y;
	private_data -> newline = last_newline;

	return ret_value;
}

int terminal_out_write_(uint8_t* buf, uint32_t length){
	int i;
	if (length == 1){
		switch (buf[0]){
			//escape sequence
			case TERMINAL_OUT_LF:
				terminal_out_newline();
				break;
			case TERMINAL_OUT_BACKSPACE:
				terminal_out_backspace();
				break;
			case TERMINAL_OUT_FF:
				terminal_out_clear();
				break;
			default:
				//otherwise, output normally
				terminal_out_putc(buf[0]);
		}
	}else {
		for (i=0; i<length; ++i){
			terminal_out_putc(buf[i]);
		}
	}
	terminal_set_cursor();
	return length;	// note for now
}

/*int terminal_out_escape_sequence(uint8_t *buf,int max_length){
	if ((buf[0]!='^')||(buf[1]!='['))
		return 0;
	// check if it is a broken escape sequence
	if (max_length>2){
		switch(buf[2]){
			case 13:
			//enter
				terminal_out_newline();
				return 3;
				break;
			case 8:
			//backspace
				terminal_out_backspace();
				return 3;
				break;
			case 12:
			//clear screen
				terminal_out_clear();
				return 3;
				break;
			default:
				// a not defined escape sequence
				return 3;
		}
	}
	// just print the broken escape sequence head ^ normally
	terminal_out_putc(buf[0]);
	return 1;
}*/

void terminal_out_putc(uint8_t c){
	int temp_offset;	//offset for video memory write

	if(c == '\n' || c == '\r') {
		// new line character
       	terminal_out_newline();
    } else {
    	// if printable
    	if (c >= ' ' && c<= '~'){
    		temp_offset = (NUM_COLS * lscreen_y + lscreen_x) << 1;
       		*(uint8_t *)(video_mem + temp_offset) = c;
       		*(uint8_t *)(video_mem + temp_offset + 1) = ATTRIB;
       		lscreen_x++;
       		lscreen_y += lscreen_x/NUM_COLS;	// characters exceed line limit
       		if (lscreen_x >= NUM_COLS){
       			// line overflow could go backspace, and this is the
       			// only case
       			lscreen_x %= NUM_COLS;
    			last_newline = 0;
    		}
       		if (lscreen_y >= NUM_ROWS){
       			// line overflow causing scroll down
       			terminal_out_scroll_down();
       			lscreen_y = NUM_ROWS-1;
       		}
       }else{
       		terminal_out_putc('.');
       }
    }

}

void terminal_out_newline(){
	last_newline = 1;	// not allowed to return to last line
	lscreen_y++;
    lscreen_x = 0;
    if (lscreen_y >= NUM_ROWS){
    	// scroll down if exceeds
       	terminal_out_scroll_down();
      	lscreen_y = NUM_ROWS-1;
    }
}

void terminal_out_backspace(){
	int temp_offset;

	lscreen_x--;
	// return to previous line check
	if (lscreen_x<0){
		if (!last_newline){
			lscreen_y --;
			lscreen_x = NUM_COLS - 1;
		}else{
			lscreen_x = 0;
		}
	}
	temp_offset = (NUM_COLS * lscreen_y + lscreen_x) << 1;
	// clear the backspaced character
    *(uint8_t *)(video_mem + temp_offset) = '\0';
    *(uint8_t *)(video_mem + temp_offset + 1) = ATTRIB;
}

void terminal_out_scroll_down(){
	int i,j,temp_offset;
	for (i=0;i<NUM_ROWS-1;++i){
		for (j=0;j<NUM_COLS;++j){
			temp_offset = (i*NUM_COLS+j)<<1;
			// memory content of next line overwrite this line
			*(uint8_t*)(video_mem + temp_offset) = *(uint8_t*)(video_mem + temp_offset + NUM_COLS*2);
			*(uint8_t*)(video_mem + temp_offset+1) = *(uint8_t*)(video_mem + temp_offset + NUM_COLS*2+1);
		}
	}
	for (j=0;j<NUM_COLS;++j){
		// set the last line to empty
		*(uint8_t*)(video_mem + NUM_COLS*(NUM_ROWS-1)*2 + j*2) = '\0';
		*(uint8_t*)(video_mem + NUM_COLS*(NUM_ROWS-1)*2 + j*2 +1) = ATTRIB;
	}
}
