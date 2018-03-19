#include "terminal_out_driver.h"

#define VIDEO       0xB8000
#define VIDEO_MEM_SIZE	0x1000
#define NUM_COLS    80
#define NUM_ROWS    25
#define ATTRIB      0x7

static int screen_x=0;
static int screen_y=0;
static int curser_x=0,curser_y=0;
static char* video_mem = (char *)VIDEO;
static int screen_stroll_offset=0;
static int last_position_before_newline;

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

void terminal_out_open(){
	//OUTB(0x3C2,)
	//screen_stroll_offset = 0;
	//outw(0x03D4, (0x00 << 8)| 0x0A);
	//outw(0x03D4, (0x08 << 8)| 0x0B);
	//int curser_temp_p = screen_y*NUM_COLS+screen_x;
	set_cursor();
}

void set_cursor(){
	int pos = screen_y * NUM_COLS + screen_x;
 
	outb(0x0F,0x3D4);
	outb((uint8_t) (pos & 0xFF), 0x3D5);
	outb(0x0E, 0x3D4);
	outb((uint8_t) ((pos >> 8) & 0xFF), 0x3D5);

}

void terminal_out_clear(){
	clear();
	screen_y = 0;
	screen_x = 0;
	curser_y = 0;
	curser_x = 0;
	video_mem = (char*)VIDEO;
	last_position_before_newline = NUM_COLS;
}

void terminal_out_write(uint8_t* buf, int length){
	int i;
	for (i=0; i<length; ++i){
		switch (buf[i]){
			//escape sequence
			case '^':
				if ((i+1<length) && buf[i+1] == '['){
					i += terminal_out_escape_sequence(buf+i,length-i)-1;
					break;
				}
			default:
				//otherwise, output normally
				terminal_out_putc(buf[i]);
		}
	}
}

int terminal_out_escape_sequence(uint8_t *buf,int max_length){
	if ((buf[0]!='^')||(buf[1]!='['))
		return 0;
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
				return 3;
		}
	}
	return 2;
}

void terminal_out_putc(uint8_t c){
	int temp_offset;
	if(c == '\n' || c == '\r') {
        terminal_out_newline();
    } else {
    	temp_offset = (NUM_COLS * screen_y + screen_x) << 1;

        *(uint8_t *)(VIDEO + temp_offset) = c;
        *(uint8_t *)(VIDEO + temp_offset + 1) = ATTRIB;
        screen_x++;
        screen_y += screen_x/NUM_COLS;
        if (screen_x >= NUM_COLS){
        	screen_x %= NUM_COLS;
    		last_position_before_newline = NUM_COLS-1;
    	}
        if (screen_y >= NUM_ROWS){
        	terminal_out_scroll_down();
        	screen_y = NUM_ROWS-1;
        }
    }
//    int curser_temp_p = screen_y*NUM_COLS+screen_x;
	set_cursor();
}

void terminal_out_newline(){
	screen_y++;
	last_position_before_newline = screen_x;
    screen_x = 0;
    if (screen_y >= NUM_ROWS){
       	terminal_out_scroll_down();
      	screen_y = NUM_ROWS-1;
    }
/*    int curser_temp_p = screen_y*NUM_COLS+screen_x;
	outw(0x03D4, ((curser_temp_p && 0x00FF)<<8)|0xF);
	outw(0x03D4, (curser_temp_p && 0xFF00)|0xE);*/
}

void terminal_out_backspace(){
	int temp_offset;
	
	screen_x--;
	if (screen_x<0){
		screen_x = last_position_before_newline;
		screen_y --;
		if (screen_y<0){
			screen_x = 0;
			screen_y = 0;
		}
	}
	temp_offset = (NUM_COLS * screen_y + screen_x) << 1;

    *(uint8_t *)(video_mem + temp_offset) = '\0';
    *(uint8_t *)(video_mem + temp_offset + 1) = ATTRIB;
    int curser_temp_p = screen_y*NUM_COLS+screen_x;
	set_cursor();
}

/*void terminal_out_scroll_down(){
	screen_stroll_offset += NUM_COLS;
	if (screen_stroll_offset >= VIDEO_MEM_SIZE){
		screen_stroll_offset -= VIDEO_MEM_SIZE;
	}
	OUTW(0x03D4, (screen_stroll_offset & 0xFF00) | 0x0C);
    OUTW(0x03D4, ((screen_stroll_offset & 0x00FF) << 8) | 0x0D);
    OUTW(0x03D4, ((NUM_ROWS-1)<<8)|0x18);
}*/


void terminal_out_scroll_down(){
	int i,j,temp_offset;
	for (i=0;i<NUM_ROWS-1;++i){
		for (j=0;j<NUM_COLS;++j){
			temp_offset = (i*NUM_COLS+j)<<1;
			*(uint8_t*)(video_mem + temp_offset) = *(uint8_t*)(video_mem + temp_offset + NUM_COLS*2); 
			*(uint8_t*)(video_mem + temp_offset+1) = *(uint8_t*)(video_mem + temp_offset + NUM_COLS*2+1); 
		}
	}
	for (j=0;j<NUM_COLS;++j){
		*(uint8_t*)(video_mem + 80*24*2 + j*2) = '\0';
		*(uint8_t*)(video_mem + 80*24*2 + j*2 +1) = ATTRIB;
	}
}
