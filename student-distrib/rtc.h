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

/**
 *	RTC_IRQ_NUM
 *
 *	IRQ number for RTC hardware.
 */
#define RTC_IRQ_NUM 8

/**
 *	RTC_PORT
 *
 *	Port for writing to RTC
 */

#define RTC_PORT    0x70

/**
 *	CMOS_PORT
 *
 *	Port for reading from RTC
 */

#define CMOS_PORT   0x71

/**
 *	REG_A
 *
 *	RTC control register A.
 */

#define REG_A       0x0A

/**
 *	REG_B
 *
 *	RTC control register B.
 */

#define REG_B       0x0B

/**
 *	REG_C
 *
 *	RTC control register C.
 */

#define REG_C       0x0C

/**
 *	REG_D
 *
 *	RTC control register D.
 */

#define REG_D       0x0D

/**
 *	REG_A_NMI
 *
 *	RTC control register A with NMI enabled (disable interrupts).
 */

#define REG_A_NMI   0x8A

/**
 *	REG_B_NMI
 *
 *	RTC control register B with NMI enabled (disable interrupts).
 */

#define REG_B_NMI   0x8B

/**
 *	REG_C_NMI
 *
 *	RTC control register C with NMI enabled (disable interrupts).
 */

#define REG_C_NMI   0x8C

/**
 *	BIT_SIX
 *
 *	the sixth bit of REG_B for enabling periodic interrupt.
 */

#define BIT_SIX     0x40

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

int32_t rtc_open();

/**
 *	Close opened rtc and free all the private data.
 *	
 *	Currently do nothing.
 */

int32_t rtc_close();

/**
 *	Block rtc interrupts at a given frequency. (virtualized)
 *
 *	While reading, process will not do anyother thing until count reach 
 *	a giving frequency.
 */

int32_t rtc_read();

/**
 *	Set frequency of RTC interrupts (virtualized)
 *
 *	@param freq: The RTC interrupt frequency (exact frequency). Must be in the range between 2-1024Hz and must be power of 2.
 */

int32_t rtc_write(int freq);

/**
 *	Check if a number is a power of 2.
 *
 *	@param freq: An input number to be checked.
 */

int is_power_of_two(int freq);

#endif /* _RTC_H */
