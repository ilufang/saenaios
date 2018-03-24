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

#define DATA_REG    0x60
#define CTRL_REG    0x64

#define KBD_IRQ_NUM 1

#define RELEASE_OFFSET 0x80

#define NULL_CHAR   '\0'

#define ENTER_P    0x1C
#define LCTRL_P    0x1D
#define LALT_P     0x38
#define BSB_P      0X0E
#define LSHIFT_P    0x2A
#define RSHIFT_P    0x36
#define CAPS_P      0x3A
#define L_P         0x26
#define ENTER_P     0x1C

#define LCTRL_R      0x9D
#define LALT_R       0xB8
#define LSHIFT_R     0xAA
#define RSHIFT_R     0xB6

/**
 *	Scan code to character mapping
 */
extern unsigned char kbdreg[128];

extern volatile int read_test_mode;

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


int32_t keyboard_read(int32_t fd, uint8_t* buf, int32_t nbytes);
int32_t keyboard_write(int32_t fd, void* buf, int32_t nbytes);
int32_t keyboard_open(const uint8_t* filename);
int32_t keyboard_close(int32_t fd);


#endif /* _RTC_H */
