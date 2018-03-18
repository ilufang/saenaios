/* keyboard.c - keyboard driver
 *	vim:ts=4 noexpandtab
 */

#include "keyboard.h"
#include "boot/idt.h"
#include "lib.h"
#define PRESSED     1
#define UNPRESSED   0
#define KEY_BUF_SIZE    128

enum key_mode{regular, caps, shift, caps_shift};

volatile enum key_mode curr_mode = regular;
volatile uint8_t ctrl_status = UNPRESSED;
volatile uint8_t shift_status = UNPRESSED;
volatile uint8_t alt_status = UNPRESSED;

volatile uint32_t curr_buf_size = 0;
volatile uint32_t prev_enter = 0;
volatile unsigned char kbd_buf[KEY_BUF_SIZE];

/* control sequences, 0 - backspace, 1 - ctrl + l */
char* ctrl_seq[2] = {"^[8", "^[12"};


/* keycode reference: Bran's Kernel development tutorial */
unsigned char kbdreg[128] =
{
	0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
	0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '`',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '/',   0,				/* Right shift */
  '*',
	0,	/* Alt */
  ' ',	/* Space bar */
	0,	/* Caps lock */
	0,	/* 59 - F1 key ... > */
	0,   0,   0,   0,   0,   0,   0,   0,
	0,	/* < ... F10 */
	0,	/* 69 - Num lock*/
	0,	/* Scroll Lock */
	0,	/* Home key */
	0,	/* Up Arrow */
	0,	/* Page Up */
  '-',
	0,	/* Left Arrow */
	0,
	0,	/* Right Arrow */
  '+',
	0,	/* 79 - End key*/
	0,	/* Down Arrow */
	0,	/* Page Down */
	0,	/* Insert Key */
	0,	/* Delete Key */
	0,   0,   0,
	0,	/* F11 Key */
	0,	/* F12 Key */
	0,	/* All other keys are undefined */
};

unsigned char kbdcaps[128] =
{
	0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'Q', 'W', 'E', 'R',	/* 19 */
  'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', '\n',	/* Enter key */
	0,			/* 29   - Control */
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';',	/* 39 */
 '\'', '`',   0,		/* Left shift */
 '\\', 'Z', 'X', 'C', 'V', 'B', 'N',			/* 49 */
  'M', ',', '.', '/',   0,				/* Right shift */
  '*',
	0,	/* Alt */
  ' ',	/* Space bar */
	0,	/* Caps lock */
	0,	/* 59 - F1 key ... > */
	0,   0,   0,   0,   0,   0,   0,   0,
	0,	/* < ... F10 */
	0,	/* 69 - Num lock*/
	0,	/* Scroll Lock */
	0,	/* Home key */
	0,	/* Up Arrow */
	0,	/* Page Up */
  '-',
	0,	/* Left Arrow */
	0,
	0,	/* Right Arrow */
  '+',
	0,	/* 79 - End key*/
	0,	/* Down Arrow */
	0,	/* Page Down */
	0,	/* Insert Key */
	0,	/* Delete Key */
	0,   0,   0,
	0,	/* F11 Key */
	0,	/* F12 Key */
	0,	/* All other keys are undefined */
};

unsigned char kbdshift[128] =
{
	0,  27, '!', '@', '#', '$', '%', '^', '&', '*',	/* 9 */
  '(', ')', '_', '+', '\b',	/* Backspace */
  '\t',			/* Tab */
  'Q', 'W', 'E', 'R',	/* 19 */
  'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',	/* Enter key */
	0,			/* 29   - Control */
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',	/* 39 */
 '"', '~',   0,		/* Left shift */
 '|', 'Z', 'X', 'C', 'V', 'B', 'N',			/* 49 */
  'M', '<', '>', '?',   0,				/* Right shift */
  '*',
	0,	/* Alt */
  ' ',	/* Space bar */
	0,	/* Caps lock */
	0,	/* 59 - F1 key ... > */
	0,   0,   0,   0,   0,   0,   0,   0,
	0,	/* < ... F10 */
	0,	/* 69 - Num lock*/
	0,	/* Scroll Lock */
	0,	/* Home key */
	0,	/* Up Arrow */
	0,	/* Page Up */
  '-',
	0,	/* Left Arrow */
	0,
	0,	/* Right Arrow */
  '+',
	0,	/* 79 - End key*/
	0,	/* Down Arrow */
	0,	/* Page Down */
	0,	/* Insert Key */
	0,	/* Delete Key */
	0,   0,   0,
	0,	/* F11 Key */
	0,	/* F12 Key */
	0,	/* All other keys are undefined */
};

unsigned char kbdcs[128] =
{
	0,  27, '!', '@', '#', '$', '%', '^', '&', '*',	/* 9 */
  '(', ')', '_', '+', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '{', '}', '\n',	/* Enter key */
	0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ':',	/* 39 */
 '"', '~',   0,		/* Left shift */
 '|', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', '<', '>', '?',   0,				/* Right shift */
  '*',
	0,	/* Alt */
  ' ',	/* Space bar */
	0,	/* Caps lock */
	0,	/* 59 - F1 key ... > */
	0,   0,   0,   0,   0,   0,   0,   0,
	0,	/* < ... F10 */
	0,	/* 69 - Num lock*/
	0,	/* Scroll Lock */
	0,	/* Home key */
	0,	/* Up Arrow */
	0,	/* Page Up */
  '-',
	0,	/* Left Arrow */
	0,
	0,	/* Right Arrow */
  '+',
	0,	/* 79 - End key*/
	0,	/* Down Arrow */
	0,	/* Page Down */
	0,	/* Insert Key */
	0,	/* Delete Key */
	0,   0,   0,
	0,	/* F11 Key */
	0,	/* F12 Key */
	0,	/* All other keys are undefined */
};

void buf_push(unsigned char c){
    if(curr_buf_size < KEY_BUF_SIZE - 1){
        curr_buf_size++;
        kbd_buf[curr_buf_size] = c;   
    }else if(c == ENTER_P && curr_buf_size == KEY_BUF_SIZE){
        curr_buf_size++;
        kbd_buf[curr_buf_size] = c;
    }
    return;
}

void clear_buf(){
    int i;
    for(i = 0; i < KEY_BUF_SIZE; i++){
        kbd_buf[i] = NULL_CHAR;
    }
}

void shift_buf(int num_bytes){
    int i;
    curr_buf_size -= num_bytes;
    prev_enter -= num_bytes;
    
    if(curr_buf_size <= 0){
        prev_enter = 0;
        curr_buf_size = 0;
        clear_buf();
        return;
    }
    
    char elements[curr_buf_size];
    
    for(i = 0; i < curr_buf_size; i++){
        elements[i] = kbd_buf[num_bytes + i];
    }
    
    clear_buf();
    
    for(i = 0; i < curr_buf_size; i++){
        kbd_buf[i] = elements[i];
    }
}

void enter(){
    curr_buf_size++;
    prev_enter = curr_buf_size;
    buf_push('\n');
}

void backspace(){
    kbd_buf[curr_buf_size] = NULL_CHAR;
    curr_buf_size--;
    (void)terminal_write(0, ctrl_seq[0], 3);
}


void keyboard_init(){
	idt_addEventListener(KBD_IRQ_NUM, &keyboard_handler);
}

int32_t keyboard_read(int32_t fd, void* buf, int32_t nbytes){
    int i = 0;
    int8_t * charbuf = (int8_t*)buf;
    for(i = 0; i < prev_enter; i++){
        charbuf[i] = kbd_buf[i];
    }
    shift_buf(prev_enter);
    return i;
}

void regular_key(unsigned char scancode){
    if(ctrl_status == PRESSED){
        if(scancode == L_P){
            /* todo: clear screen */
            /*
            set_cursor(0,0);
            clear_buf(curr_buf_size);
            clear();
            */
            (void)terminal_write(0, ctrl_seq[2], 3);
            return;
        }
    }
    unsigned char scanchar;
    if(scancode&0x80) return;
    switch(curr_mode){
        case regular:
            scanchar = kbdreg[scancode];
            break;
        case caps:
            scanchar = kbdcaps[scancode];
            break;
        case shift:
            scanchar = kbdshift[scancode];
            break;
        case caps_shift:
            scanchar = kbdcs[scancode];
            break;         
    }
    buf_push(scanchar);
    //putc(scanchar);
    (void)terminal_write(1, &scanchar, 1);
}

void update_mode(unsigned char scancode){
        switch(scancode){
            case LSHIFT_P:
            case RSHIFT_P:
                shift_status = PRESSED;
                if(curr_mode == caps){
                    curr_mode = caps_shift;
                }
                else{
                    curr_mode = shift;
                }
                break;
            case LSHIFT_R:
            case RSHIFT_R:
                shift_status = UNPRESSED;
                if(curr_mode == caps_shift){
                    curr_mode = caps;
                }
                else{
                    curr_mode = regular;
                }
                break;
            case CAPS_P:
                switch(curr_mode){
                    case caps:
                        curr_mode = regular;
                        break;
                    case regular:
                        curr_mode = caps;
                        break;
                    case shift:
                        curr_mode = caps_shift;
                        break;
                    case caps_shift:
                        curr_mode = caps;
                        break;
                }
                break;
        }

}



void keyboard_handler(){
	unsigned char scancode;
	/* reads scancode */
	scancode = inb(DATA_REG);
	/* put the corresponding character on screen */
	switch(scancode){
        case ENTER_P:
            enter();
            break;
        case LSHIFT_R:
        case LSHIFT_P:
        case RSHIFT_R:
        case RSHIFT_P:
        case CAPS_P:
            update_mode(scancode);
            break;
            
        case LCTRL_P:
            ctrl_status = PRESSED;
            break;
        case LCTRL_R:
            ctrl_status = UNPRESSED;
            break;
        case LALT_P:
            alt_status = PRESSED;
            break;
        case LALT_R:
            alt_status = UNPRESSED;
            break;
        
        case BSB_P:
            backspace();
            break;
        
        default:
            regular_key(scancode);
            break;
    }
    
	send_eoi(KBD_IRQ_NUM);
}
