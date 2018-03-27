#include "terminal_out_driver.h"

static int screen_x=0, screen_y=0;	//x,y position on the screen
static int curser_x=0, curser_y=0;	//not useful for now
//pointing to start address of video memory
static char* video_mem = (char *)VIDEO;	
// scroll offset initialized to 0
//static int screen_stroll_offset=0;
// if last_newline is 1, then cannot backspace to last line
static int last_newline = 0;

static file_operations_t terminal_out_op;

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
	terminal_out_op.read = & terminal_out_read;
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

void terminal_set_cursor(){
	// calculate memory position for cursor position
	int pos = screen_y * NUM_COLS + screen_x;
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
	screen_y = 0;
	screen_x = 0;
	curser_y = 0;
	curser_x = 0;
	last_newline = 1;

	terminal_set_cursor();
}

ssize_t terminal_out_write(file_t* file, uint8_t* buf,size_t count,off_t* offset){
	return (ssize_t)(terminal_out_write_(buf,(int)count));	// wrap for now
}

int terminal_out_write_(uint8_t* buf, int length){
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
	return length;	// note for now
}

ssize_t terminal_out_read(file_t* file, uint8_t* buf,size_t count,off_t* offset){
	return 0;	// for now
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
    		temp_offset = (NUM_COLS * screen_y + screen_x) << 1;
       		*(uint8_t *)(VIDEO + temp_offset) = c;
       		*(uint8_t *)(VIDEO + temp_offset + 1) = ATTRIB;
       		screen_x++;
       		screen_y += screen_x/NUM_COLS;	// characters exceed line limit
       		if (screen_x >= NUM_COLS){
       			// line overflow could go backspace, and this is the
       			// only case
       			screen_x %= NUM_COLS;
    			last_newline = 0;
    		}
       		if (screen_y >= NUM_ROWS){
       			// line overflow causing scroll down
       			terminal_out_scroll_down();
       			screen_y = NUM_ROWS-1;
       		}
       		terminal_set_cursor();
       }else{
       		terminal_out_putc('.');
       }
    }

}

void terminal_out_newline(){
	last_newline = 1;	// not allowed to return to last line
	screen_y++;
    screen_x = 0;
    if (screen_y >= NUM_ROWS){
    	// scroll down if exceeds
       	terminal_out_scroll_down();
      	screen_y = NUM_ROWS-1;
    }
	terminal_set_cursor();
}

void terminal_out_backspace(){
	int temp_offset;
	
	screen_x--;
	// return to previous line check
	if (screen_x<0){
		if (!last_newline){
			screen_y --;
			screen_x = NUM_COLS - 1;
		}else{
			screen_x = 0;
		}
	}
	temp_offset = (NUM_COLS * screen_y + screen_x) << 1;
	// clear the backspaced character
    *(uint8_t *)(video_mem + temp_offset) = '\0';
    *(uint8_t *)(video_mem + temp_offset + 1) = ATTRIB;
	terminal_set_cursor();
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
	terminal_set_cursor();
}
