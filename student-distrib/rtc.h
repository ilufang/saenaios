/* rtc.h - Header file for rtc driver
 * vim:ts=4 noexpandtab
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

#define REG_A       0x8A
#define REG_B       0x8B
#define REG_C       0x8C
#define REG_D       0x8D

#define BIT_SIX     0x40

/* Externally-visible functions */

/* Initialize the rtc */
void rtc_init();
/* interrupt handler */
void rtc_handler();

#endif /* _RTC_H */
