/* keyboard.c - keyboard driver
 *	vim:ts=4 noexpandtab
 */

#include "keyboard.h"
#include "boot/idt.h"
#include "lib.h"

#define PRESSED     1
#define UNPRESSED   0
#define KEY_BUF_SIZE    128

#define SEQ_BSB     0
#define SEQ_CLR     1
#define SEQ_ENTER   2

// whether the keyboard is in read_test mode
volatile int read_test_mode = 0;

// four key modes
enum key_mode{regular, caps, shift, caps_shift};

// keyboard status variables
volatile enum key_mode curr_mode = regular;
volatile uint8_t ctrl_status = UNPRESSED;
volatile uint8_t shift_status = UNPRESSED;
volatile uint8_t alt_status = UNPRESSED;

// indices into keyboard buffer
int curr_char_ptr = 0;
int prev_enter = -1;

// keyboard buffer
uint8_t kbd_buf[KEY_BUF_SIZE];

/* control sequences for keyboard-terminal communication
    0 - backspace, 1 - ctrl + l, 2 - enter */
const uint8_t seq_backspace[3] = {'^', '[', 8};
const uint8_t seq_ctrl_l[3] = {'^', '[', 12};
const uint8_t seq_enter[3] = {'^', '[', 13};


/* keycode lookup table for 4 keyboard modes */
/* keycode reference: Bran's Kernel development tutorial */
uint8_t kbdreg[128] =
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

uint8_t kbdcaps[128] =
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

uint8_t kbdshift[128] =
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

uint8_t kbdcs[128] =
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

/**
 *	Helper function to append a char keyboard buffer
 *
 *  Modify keyboard buffer and update current char pointer
 *  If there are 127 elements in the buffer and the new key 
 *  is not enter, do nothing.
 *
 *	@param c: the char to add to buffer
 */
void buf_push(uint8_t c){
    if(curr_char_ptr < KEY_BUF_SIZE){
        kbd_buf[curr_char_ptr] = c;   
        curr_char_ptr++;
    }else if(c == ENTER_P && curr_char_ptr == KEY_BUF_SIZE - 1){
        kbd_buf[curr_char_ptr] = c;
        curr_char_ptr++;
    }
    return;
}


/**
 *	Helper function to clear keyboard buffer
 *
 *  Clears keyboard buffer and reset buffer pointers
 */
void clear_buf(){
    int i;
    for(i = 0; i < KEY_BUF_SIZE; i++){
        kbd_buf[i] = NULL_CHAR;
    }
    curr_char_ptr = 0;
    prev_enter = -1;
}


/**
 *	Helper function to shift keyboard buffer
 *
 *  Left shift keyboard buffer, discart data at the front
 *
 *	@param num_bytes: number of characters to shift
 */
void shift_buf(int num_bytes){
    int i;
    curr_char_ptr -= num_bytes;
    prev_enter -= num_bytes;
    
    if(curr_char_ptr < 0){
        prev_enter = -1;
        curr_char_ptr = 0;
        clear_buf();
        return;
    }
    
    uint8_t elements[curr_char_ptr];
    
    for(i = 0; i < curr_char_ptr; i++){
        elements[i] = kbd_buf[num_bytes + i];
    }
    
    clear_buf();
    
    for(i = 0; i < curr_char_ptr; i++){
        kbd_buf[i] = elements[i];
    }
}


/**
 *	Helper function handle enter key press
 *
 *  Update position and push an enter char to keyboard buffer, send enter
 *  control sequence to terminal
 *
 *  @note in test_read mode, the function also read from the current
 *        keyboard buffer and print to the screen
 */
void enter(){
    if(curr_char_ptr < KEY_BUF_SIZE){
        prev_enter = curr_char_ptr;
        buf_push('\n'); 
        //(void)write(1, seq_enter, 3);
        terminal_out_write((uint8_t*)seq_enter, 3);
    }
    if(read_test_mode == 1){
        uint8_t testbuf[prev_enter];
        int fd, nbytes;
        int32_t num_read;
        num_read = keyboard_read(fd, testbuf, nbytes);
        terminal_out_write(testbuf, num_read);
        if(testbuf[0]=='`'){
            read_test_mode = 0;
        }
    }   
}

/**
 *	Helper function to handle backspace keypress
 *
 *  Erase one character in the keyboard buffer, unless an enter is reached.
 *  Send backspace control sequence to terminal
 */
void backspace(){
    if(curr_char_ptr>0 && kbd_buf[curr_char_ptr-1]!='\n'){
        curr_char_ptr--;
        kbd_buf[curr_char_ptr] = NULL_CHAR;  
        terminal_out_write((uint8_t*)seq_backspace,3);
    }
    //(void)write(1, seq_backspace, 3);
}

/**
 *	Helper function to handle ctrl+l keypress combination
 *
 *  Sends ctrl_l control sequence to terminal
 *
 *  @note the function does not erase characters entered but not yet read
 *        in keyboard buffer
 */
void ctrl_l(){
    //(void)write(1, seq_ctrl_l, 3);  
    terminal_out_write((uint8_t*)seq_ctrl_l,3);
}


/**
 *	Helper function to handle regular keypress
 *  
 *  Send a character to terminal. Check for ctrl_l
 *
 *  @param scancode: scancode received
 */
void regular_key(uint8_t scancode){
    // check for possible ctrl-l if control key is pressed
    if(ctrl_status == PRESSED){
        if(scancode == L_P){
            /* todo: clear screen */
            /*
            set_cursor(0,0);
            clear_buf(curr_char_ptr);
            clear();
            */
            ctrl_l();
            return;
        }
    }
    // for other regular keys just grab the char from lookup table
    uint8_t scanchar;
    // do nothing if the key is released
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
    if (scanchar == 0 ) return;
    
    if(curr_char_ptr < KEY_BUF_SIZE-1){
        // write the character to terminal
        buf_push(scanchar);
        //putc(scanchar);
        uint8_t keybuf[1];
        keybuf[0] = scanchar;
        //(void)write(1, keybuf, 1);
        terminal_out_write(keybuf,1);
    }
}

/**
 *	Helper function to handle shift and capslock
 *  
 *  Change the current keyboard mode
 *
 *  @param scancode: scancode received
 */
void update_mode(uint8_t scancode){
        switch(scancode){
            // update shift press/release status
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
            // update capslock status
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
	uint8_t scancode;
	/* reads scancode */
	scancode = inb(DATA_REG);
	switch(scancode){
        // check whether we need to handle enter
        case ENTER_P:
            enter();
            break;

        // check if it's necessary to update keyboard mode
        case LSHIFT_R:
        case LSHIFT_P:
        case RSHIFT_R:
        case RSHIFT_P:
        case CAPS_P:
            update_mode(scancode);
            break;
            
        // update keyboard status variables
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
        
        // check if we need to handle backspace
        case BSB_P:
            backspace();
            break;
        
        default:
            regular_key(scancode);
            break;
    }
    
	send_eoi(KBD_IRQ_NUM);
}


void keyboard_init(){
	idt_addEventListener(KBD_IRQ_NUM, &keyboard_handler);
}

int32_t keyboard_read(int32_t fd, uint8_t* buf, int32_t nbytes){
    if(prev_enter < 0){
        return -1;
    }
    int i = 0;
    for(i = 0; i <= prev_enter; i++){
        buf[i] = kbd_buf[i];
    }
    shift_buf(prev_enter);
    return i;
}

int32_t keyboard_write(int32_t fd, void* buf, int32_t nbytes){
    return -1;
}

int32_t keyboard_open(const uint8_t* filename){
    clear_buf();
    return 0;
}

int32_t keyboard_close(int32_t fd){
    clear_buf();
    return 0;
}

