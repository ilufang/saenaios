/* rtc.h - Header file for rtc driver
 * vim:ts=4 noexpandtab
 */

#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "types.h"
#include "i8259.h"
#include "lib.h"

#define DATA_REG    0x60
#define CTRL_REG    0x64

#define KBD_IRQ_NUM 1

/* Externally-visible functions */

/* Initialize the keyboard */
void keyboard_init();
/* interrupt handler */
void keyboard_handler();

#endif /* _RTC_H */
