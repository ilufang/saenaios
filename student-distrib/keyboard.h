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

#define DATA_REG    0x60
#define CTRL_REG    0x64

#define KBD_IRQ_NUM 1

#define RELEASE_OFFSET 0x80

/**
 *	Scan code to character mapping
 */
extern unsigned char kbdus[128];

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

#endif /* _RTC_H */
