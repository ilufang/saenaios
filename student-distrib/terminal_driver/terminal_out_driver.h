#ifndef _TERMINAL_DRIVER_H
#define _TERMINAL_DRIVER_H

#include "../types.h"
#include "../lib.h"

extern void terminal_out_write(uint8_t* buf, int length);

void terminal_out_newline();

void terminal_out_backspace();

int terminal_out_escape_sequence(uint8_t *buf,int max_length);

void terminal_out_clear();

void terminal_out_putc(uint8_t c);

void terminal_out_escape(uint8_t* buf);

void terminal_out_scroll_down();

void terminal_out_open();

#endif
