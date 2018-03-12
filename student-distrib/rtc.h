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
#define CMOS_PORT   0x71

#define REG_A       0x0A
#define REG_B       0x0B
#define REG_C       0x0C
#define REG_D       0x0D

#define REG_A_NMI   0x8A
#define REG_B_NMI   0x8B
#define REG_C_NMI   0x8C

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
 *	Currently only calls test_interrupte() to make sure that we are able to
 *	receive interrupt. Changes video memory content
 */
void rtc_handler();

/**
 *	Set rate of RTC interrupts
 *
 *	@param rate: The RTC interrupt rate (divisions)
 */
void rtc_setrate(int rate);

#endif /* _RTC_H */
