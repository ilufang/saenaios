/**
 *	@file rtc.h
 *
 *	Header file for rtc driver
 *
 *	vim:ts=4 noexpandtab
 */

#ifndef _RTC_H
#define _RTC_H

#include "types.h"
#include "i8259.h"

/* irq number for rtc */
#define RTC_IRQ_NUM 8

/* rtc ports */
#define RTC_PORT    0x70
/**
 *	RTC_PORT
 *
 *	Port for writing to RTC
 */
#define CMOS_PORT   0x71
/**
 *	CMOS_PORT
 *
 *	Port for reading from RTC
 */
#define REG_A       0x0A
#define REG_B       0x0B
#define REG_C       0x0C
#define REG_D       0x0D

#define REG_A_NMI   0x8A
#define REG_B_NMI   0x8B
#define REG_C_NMI   0x8C

#define BIT_SEVEN	0x80
#define BIT_SIX     0x40
#define BIT_FIVE 	0x20
#define BIT_FOUR	0x10
#define BIT_THREE	0x08
#define BIT_TWO 	0x04
#define BIT_ONE 	0x02

/**
 *	Initialize the rtc
 *
 *	Writes to rtc control register, enables irq8 on PIC
 */
void rtc_init();

/**
 *	Temporary rtc interrupt handler
 *
 *	Currently only calls test_interrupts() to make sure that we are able to
 *	receive interrupt. Changes video memory content
 */
void rtc_handler();

/**
 *	Temporary rtc interrupt handler
 *
 *	Currently only called by rtc_test_2 to check rtc functionality.
 *
 */
void test_rtc_handler();

/**
 *	Set rate of RTC interrupts
 *
 *	@param rate: The RTC interrupt rate (divisions)
 */
void rtc_setrate(int rate);

/**
 *	Initialize frequency to 2 Hz and enable RTC by changing rtc_status. 
 *
 *	Occured when RTC needs to be turned on.
 */

int rtc_open();

/**
 *	Close opened rtc and free all the private data.
 *	
 *	Currently do nothing.
 */

int rtc_close();

/**
 *	Block rtc interrupts at a given frequency. (virtualized)
 *
 *	While reading, process will not do anyother thing until count reach 
 *	a giving frequency.
 */

int rtc_read();

/**
 *	Set frequency of RTC interrupts (virtualized)
 *
 *	@param freq: The RTC interrupt frequency (exact frequency). Must be in the range between 2-1024Hz and must be power of 2.
 */

int rtc_write(int freq);

/**
 *	Check if a number is a power of 2.
 *
 *	@param freq: An input number to be checked.
 */

int is_power_of_two(int freq);

#endif /* _RTC_H */
