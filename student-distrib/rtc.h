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

/**
 *	Initialize frequency to 2 Hz and enable RTC
 *
 *	Occured when RTC needs to be turned on and initialized.
 */

int rtc_open();

int rtc_read();

int rtc_write();

#endif /* _RTC_H */

/**
 * 	Reg A:

 *	Bit 7 UIP: (RD only)
 *	Write to SET bit in Reg B to 1 inhibit any update tranfer and clears the UIP(Reg A bit 7) status bit.

 * 	Bit 6, 5, and 4: DV2, DV1, DV0:

 *	010 of Bits 6, 5 and 4 of Reg A(only combination to turn on oscillator on and allow the RTC to keep time).
 *	A pattern of 11x enables the oscillator on but holds the countdown chain in reset.
 *	The next update occurs at 500ms after a pattern of 010 is written to DV0, DV1, and DV2.

 *	Bit 3 to 0: Rate Selector (See ds12885/87 Table 3)

 *	Reg B:

 *	Bit 7: SET:
 *	Bit 6: Periodic Interrupt Enable (PIE):
 *	Bit 5: Alarm Interrupt Enable (AIE): 
 *	Bit 4: Update-Ended Interrupt Enable (UIE):
 *	Bit 3: Square-Wave Enable (SQWE):
 *	Bit 2: Data Mode (DM):
 *	Bit 1: 24/12
 *	Bit 0: Daylight Saving Enable (DSE):

 *	Reg C:

 *	Interrupt Request Flag
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *

 */
