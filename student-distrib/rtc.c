/* rtc.c - linux rtc driver
 * vim:ts=4 noexpandtab
 */
#include "rtc.h"
#include "boot/idt.h"
#include "lib.h"

void rtc_init(){
	/* selects register B */
	outb(REG_B_NMI, RTC_PORT);
	/* read and store current value */
	unsigned char prev = inb(CMOS_PORT);
	/* set the register to register B again */
	outb(REG_B_NMI, RTC_PORT);
	/* turns on bit 6 of register B */
	outb(prev | BIT_SIX, CMOS_PORT);
	/* enable the corresponding irq line on PIC */
	idt_addEventListener(RTC_IRQ_NUM, &rtc_handler);
}

void rtc_handler(){
	/* sends eoi */
	send_eoi(RTC_IRQ_NUM);
	/* reads from register C so that the interrupt will happen again */
	outb(REG_C, RTC_PORT);
	inb(CMOS_PORT);
	/* Not functional yet */
}

void rtc_setrate(int rate) {
	char prev;

	if (rate <= 2 || rate >= 16)
		return;

	disable_irq(RTC_IRQ_NUM);
	outb(REG_A_NMI, RTC_PORT);		// set index to register A, disable NMI
	prev = inb(CMOS_PORT);			// get initial value of register A
	outb(REG_A_NMI, RTC_PORT);		// reset index to A
	outb((prev & 0xF0) | rate, CMOS_PORT); //write only our rate to A.
	enable_irq(RTC_IRQ_NUM);
}
