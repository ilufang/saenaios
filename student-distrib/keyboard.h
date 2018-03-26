/**
 *	@file keyboard.h
 *
 *	Header file for keyboard driver
 *
 *	vim:ts=4 noexpandtab
 */

#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "types.h"
#include "i8259.h"
#include "lib.h"
#include "terminal_driver/terminal_out_driver.h"

#include "fs/vfs.h"
#include "fs/fs_devfs.h"

#define DATA_REG    0x60		///< keyboard data register
#define CTRL_REG    0x64		///< keyboard control register

#define KBD_IRQ_NUM 1			///< keyboard irq number

#define RELEASE_OFFSET 0x80		///< higher byte offset for key release

#define NULL_CHAR   '\0'		///< null character
#define ENTER_CHAR   '\n'		///< enter character
#define BSB_CHAR   '\b'		///< backspace character

#define ENTER_P    0x1C			///< keycode enter pressed
#define LCTRL_P    0x1D			///< keycode left control pressed
#define LALT_P     0x38			///< keycode left alt pressed
#define BSB_P      0X0E			///< keycode backspace pressed
#define LSHIFT_P    0x2A		///< keycode left shift pressed
#define RSHIFT_P    0x36		///< keycode right shift pressed
#define CAPS_P      0x3A		///< keycode capslock pressed
#define L_P         0x26		///< keycode l pressed
#define ENTER_P     0x1C		///< keycode enter pressed

#define LCTRL_R      0x9D		///< keycode left control released
#define LALT_R       0xB8		///< keycode left alt released
#define LSHIFT_R     0xAA		///< keycode left shift released
#define RSHIFT_R     0xB6		///< keycode right shift released

#define KEY_BUF_SIZE    128		///< max keyboard buffer size

/**
 *	Scan code to character mapping
 */
extern unsigned char kbdreg[KEY_BUF_SIZE];

/**
 *	Initializes keyboard
 */
void keyboard_init();

/**
 *	Keyboard Handler
 *
 *	Output characters to screen.
 *	@note only echoes numbers and lower case letter on screen
 */
void keyboard_handler();

/**
 *	Register keyboard driver in devfs
 */
int keyboard_driver_register();

/**
 *	Keyboard read wrapper
 *
 *	@param file: the file obj
 *	@param buf: buffer to read into
 *	@param count: number of byte to read
 *	@param offset: offset to start reading
 */
ssize_t keyboard_read(file_t* file, uint8_t *buf, size_t count, off_t *offset);

/**
 *	Keyboard write function
 *
 *	@note keyboard write is not supported
 */
ssize_t keyboard_write(file_t* file, uint8_t *buf, size_t count, off_t *offset);

/**
 *	Keyboard open function
 *
 *	Initialize any local variable
 *
 *	@param file: file object
 *
 */
int32_t keyboard_open(inode_t* inode, file_t* file);

/**
 *	Keyboard close function
 *
 *	@note the function does nothing for now
 *	@param inode: inode of the file to close
 *	@param fd: index into file descriptor table
 */
int32_t keyboard_close(inode_t* inode, file_t* file);


#endif /* _KEYBOARD_H */
