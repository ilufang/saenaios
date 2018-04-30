/* keyboard.c - keyboard driver
 *  vim:ts=4 noexpandtab
 */

#include "keyboard.h"
#include "boot/idt.h"
#include "lib.h"
#include "proc/task.h"
#include "proc/signal.h"

#define PRESSED     1			///< key state pressed
#define UNPRESSED   0			///< key state unpressed

/*#define TERMINAL_ONE	104
#define TERMINAL_TWO	105
#define TERMINAL_THR	106
#define TERMINAL_FUR 	107*/

#define TERMINAL_OFFSET (-104)
/**
 *	Keyboard modes corresponding to capslock and shift state
 */
enum key_mode{regular, caps, shift, caps_shift};


/**
 *	Keyboard status variable for current key mode
 */
volatile enum key_mode curr_mode = regular;

/**
 *	Keyboard status variable for control key status
 */
volatile uint8_t ctrl_status = UNPRESSED;

/**
 *	Keyboard status variable for shift key status
 */
volatile uint8_t shift_status = UNPRESSED;

/**
 *	Keyboard status variable for alt key status
 */
volatile uint8_t alt_status = UNPRESSED;

/**
 *	Index of current character in key buffer
 */
volatile int curr_char_ptr = 0;

/**
 *	Index of previous enter in key buffer
 */
volatile int prev_enter = -1;

/**
 *	Keyboard buffer for user input
 */
uint8_t kbd_buf[KEY_BUF_SIZE];

/**
 *	keyboard driver file operations
 */
static file_operations_t keyboard_fop;

/**
 *	keycode lookup table for regular keyboard mode
 *	keycode reference: Bran's Kernel development tutorial
 */
uint8_t kbdreg[128] =
{
  0,  27, '1', '2', '3', '4', '5', '6', '7', '8', /* 9 */
  '9', '0', '-', '=', '\b', /* Backspace */
  '\t',     /* Tab */
  'q', 'w', 'e', 'r', /* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', /* Enter key */
  0,      /* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', /* 39 */
 '\'', '`',   0,    /* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',      /* 49 */
  'm', ',', '.', '/',   0,        /* Right shift */
	'*',
  0,  /* Alt */
  ' ',  /* Space bar */
  0,  /* Caps lock */
  0,  /* 59 - F1 key ... > */
	0,   0,   0,   0,   0,   0,   0,   0,
  0,  /* < ... F10 */
  0,  /* 69 - Num lock*/
  0,  /* Scroll Lock */
  0,  /* Home key */
  0,  /* Up Arrow */
  0,  /* Page Up */
	'-',
  0,  /* Left Arrow */
	0,
  0,  /* Right Arrow */
	'+',
  0,  /* 79 - End key*/
  0,  /* Down Arrow */
  0,  /* Page Down */
  0,  /* Insert Key */
  0,  /* Delete Key */
	0,   0,   0,
  0,  /* F11 Key */
  0,  /* F12 Key */
  0,  /* All other keys are undefined */
};


/**
 *	keycode lookup table for capslock keyboard mode
 *	keycode reference: Bran's Kernel development tutorial
 */
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


/**
 *	keycode lookup table for shift keyboard mode
 *	keycode reference: Bran's Kernel development tutorial
 */
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


/**
 *	keycode lookup table for capslock + shift keyboard mode
 *	keycode reference: Bran's Kernel development tutorial
 */
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
 *	keycode lookup table for capslock + shift keyboard mode
 *	keycode reference: Bran's Kernel development tutorial
 */
uint8_t kbdctl[128] =
{
	0,  27, '!', '@', '#', '$', '%', 30, '&', '*',	/* 9 */
  '(', ')', 31, '+', '\b',	/* Backspace */
  '\t',			/* Tab */
  17, 23, 5, 18,	/* 19 */
  20, 25, 21, 9, 15, 16, 27, 29, '\n',	/* Enter key */
	0,			/* 29   - Control */
  1, 19, 4, 6, 7, 8, 10, 11, 12, ':',	/* 39 */
 '"', '~',   0,		/* Left shift */
 28, 26, 24, 3, 22, 2, 14,			/* 49 */
  13, '<', '>', '?',   0,				/* Right shift */
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
 *	keycode lookup table for alt pressed
 *	keycode reference: http://webpages.charter.net/danrollins/techhelp/0055.HTM
 */
uint8_t kbdalt[128] =
{
  0,  27,  120, 121, 122, 123, 124, 125, 126, 127, /* 9 */
  128, 129, 130, 131, 14, /* Backspace */
  165,     /* Tab */
  16, 17, 18, 19, /* 19 */
  20, 21, 22, 23, 24, 25, 26, 27, 28, /* Enter key */
  0,      /* 29   - Control */
  30, 31, 32, 33, 34, 35, 36, 37, 38, 39, /* 39 */
  40, 41,   0,    /* Left shift */
  43, 44, 45, 46, 47, 48, 49,      /* 49 */
  50, 51, 52, 53,   0,        /* Right shift */
	'*',
  0,  /* Alt */
  ' ',  /* Space bar */
  0,  /* Caps lock */
  104,  /* 59 - F1 key ... > */
	105,   106,   107,   108,   109,   110,   111,   112,
  113,  /* < ... F10 */
  0,  /* 69 - Num lock*/
  0,  /* Scroll Lock */
  0,  /* Home key */
  0,  /* Up Arrow */
  0,  /* Page Up */
	'-',
  0,  /* Left Arrow */
	0,
  0,  /* Right Arrow */
	'+',
  0,  /* 79 - End key*/
  0,  /* Down Arrow */
  0,  /* Page Down */
  0,  /* Insert Key */
  0,  /* Delete Key */
	0,   0,   0,
  139,  /* F11 Key */
  140,  /* F12 Key */
  0,  /* All other keys are undefined */
};

/// PID that is awaiting input
pid_t keyboard_pid_waiting;

/**
 *	Helper function to append a char keyboard buffer
 *
 *	Modify keyboard buffer and update current char pointer
 *	If there are 127 elements in the buffer and the new key
 *	is not enter, do nothing.
 *
 *	@param c: the char to add to buffer
 */
void buf_push(uint8_t c){
	if(curr_char_ptr < KEY_BUF_SIZE){
		kbd_buf[curr_char_ptr] = c;
		curr_char_ptr++;
	}else if(c == ENTER_CHAR && curr_char_ptr == KEY_BUF_SIZE - 1){
		kbd_buf[curr_char_ptr] = c;
		curr_char_ptr++;
	}
	return;
}


/**
 *	Helper function to clear keyboard buffer
 *
 *	Clears keyboard buffer and reset buffer pointers
 */
void clear_buf(){
	int i;
	// set all character in buffer to null
	for(i = 0; i < KEY_BUF_SIZE; i++){
		kbd_buf[i] = NULL_CHAR;
	}
	// reset buffer indices
	curr_char_ptr = 0;
	prev_enter = -1;
}


/**
 *	Helper function to shift keyboard buffer
 *
 *	Left shift keyboard buffer, discart data at the front
 *
 *	@param num_bytes: number of characters to shift
 */
void shift_buf(int num_bytes){
	int i;
	// update buffer indices
	curr_char_ptr -= num_bytes;
	prev_enter -= num_bytes;

	// if numbytes is greater than current byte stored in buffer, clear buffer
	if(curr_char_ptr < 0){
		prev_enter = -1;
		curr_char_ptr = 0;
		clear_buf();
		return;
	}

	// shift data in buffer
	uint8_t elements[curr_char_ptr];

	for(i = 0; i < curr_char_ptr; i++){
		elements[i] = kbd_buf[num_bytes + i];
	}

	clear_buf();
	// store shifted data
	for(i = 0; i < curr_char_ptr; i++){
		kbd_buf[i] = elements[i];
	}
}

/**
 *	Helper function to handle regular keypress
 *
 *	Send a character to terminal
 *
 *	@param scancode: scancode received
 */
void regular_key(uint8_t scancode){
	// do nothing if the key is released
	if(scancode&RELEASE_OFFSET) return;

	uint8_t scanchar;

	// send control character if control key is pressed
	if(alt_status == PRESSED){
		scanchar = kbdalt[scancode];
		/*
		uint8_t scanchar_buf[2];
		scanchar_buf[0] = 0;
		scanchar_buf[1] = scanchar;
		tty_send_input((uint8_t*)scanchar_buf, 2);*/
		/*switch(scanchar){
			case TERMINAL_ONE:
			//something
			break;
			case TERMINAL_TWO:
			//something
			break;
			case TERMINAL_THR:
			//something
			break;
		//TODO: switch terminal or something like that?
		}*/
		scanchar += TERMINAL_OFFSET;
		if (scanchar < MAX_STDOUT)
			tty_switch(scanchar);
		return;
	}
	else if(ctrl_status == PRESSED){
		scanchar = kbdctl[scancode];
		tty_send_input(&scanchar, 1);
		return;
	}
	else{
	// otherwise fetch a scancode according to current mode
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
	}

	tty_send_input(&scanchar, 1);
	/*
	switch(scanchar){
		// if enter is pressed, mark the previous enter position
		case ENTER_CHAR:
		if(curr_char_ptr < KEY_BUF_SIZE){
			// update enter location
			prev_enter = curr_char_ptr + 1;
			// append enter in keyboard buffer
			buf_push(scanchar);
			// write enter control sequence to terminal driver
			terminal_out_write_(&scanchar, 1);
			// Signal if there are process awaiting
			if (keyboard_pid_waiting) {
				syscall_kill(keyboard_pid_waiting, SIGIO, 0);
				keyboard_pid_waiting = 0;
			}
		}
		break;

		// if backspace is pressed, erase a char from key buffer
		case BSB_CHAR:
		if(curr_char_ptr>0 && kbd_buf[curr_char_ptr-1]!='\n'){
			curr_char_ptr--;
			kbd_buf[curr_char_ptr] = NULL_CHAR;
				// send control sequence to terminal driver
			terminal_out_write_(&scanchar,1);
		}
		break;

		// in other cases just sent the keycode
		default:
		if(curr_char_ptr < KEY_BUF_SIZE-1){
				// write the character to terminal
			buf_push(scanchar);
			terminal_out_write_(&scanchar,1);
		}
		break;
	}
	*/
}

/**
 *	Helper function to handle shift and capslock
 *
 *	Change the current keyboard mode
 *
 *	@param scancode: scancode received
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
		// check if it's necessary to update keyboard mode
		case LSHIFT_R:
		case LSHIFT_P:
		case RSHIFT_R:
		case RSHIFT_P:
		case CAPS_P:
		update_mode(scancode);
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

int keyboard_driver_register(){
	// fill file operation table
	keyboard_fop.open = &keyboard_open;
	keyboard_fop.release = &keyboard_close;
	keyboard_fop.read = &keyboard_read;
	keyboard_fop.write = &keyboard_write;
	keyboard_fop.readdir = NULL;
	return (devfs_register_driver("stdin", &keyboard_fop));
}

ssize_t keyboard_read(file_t* file, uint8_t *buf, size_t count, off_t *offset){
	task_sigact_t sa;
	sigset_t ss;
	
	if (prev_enter < 0){
		// No enter in buffer, set process to sleep until SIGIO
		keyboard_pid_waiting = task_current_pid();
		sa.handler = SIG_IGN;
		sigemptyset(&(sa.mask));
		sa.flags = SA_RESTART;
		syscall_sigaction(SIGIO, (int)&sa, 0);
		sigemptyset(&ss);
		syscall_sigsuspend((int) &ss, NULL, 0);
		return 0; // Should not hit
	}

	if (prev_enter < (int) count) {
		// If the flushable part of internal buffer is smaller than the external
		// buffer, only copy the length of the flushable part of internal buffer
		count = prev_enter;
	}
	memcpy(buf, kbd_buf, count);
	shift_buf(count);
	return count;
}

ssize_t keyboard_write(file_t* file, uint8_t *buf, size_t count, off_t *offset){
	return -ENOSYS;
}

int32_t keyboard_open(inode_t* inode, file_t* file){
	clear_buf();
	return 0;
}

int32_t keyboard_close(inode_t* inode, file_t* file){
	clear_buf();
	return 0;
}

