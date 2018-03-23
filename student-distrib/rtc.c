/* rtc.c - linux rtc driver
 * vim:ts=4 noexpandtab
 */
#include "rtc.h"
#include "boot/idt.h"
#include "lib.h"

static unsigned char block = 0;

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
	block = 0;
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

/* need to virtualization rtc behaviors */
//TODO

int rtc_open() {
	char prev;
	/* avoid new interrupts come in */
	disable_irq(RTC_IRQ_NUM);
	/* select register A and disbale NMI */
	outb(REG_A_NMI, RTC_PORT);
	/* read and store current value */
	prev = inb(CMOS_PORT);
	/* select register A again */
	outb(REG_A_NMI, RTC_PORT);
	/* set the 0-3 bits to adjust frequency to 2Hz */
	outb(((prev & 0xF0) | 0x0F), CMOS_PORT);
	/* selects register B */
	outb(REG_B_NMI, RTC_PORT);
	/* read and store current value */
	prev = inb(CMOS_PORT);
	/* set the register to register B again */
	outb(REG_B_NMI, RTC_PORT);
	/* turns on bit 6 of register B to enable PIE */
	outb(prev | BIT_SIX, CMOS_PORT);
	/* enable new interrupts come in */
	enable_irq(RTC_IRQ_NUM);
	/* enable the corresponding irq line on PIC */
	idt_addEventListener(RTC_IRQ_NUM, &rtc_handler);
	/* return 0 for successful open */
	return 0;
}

int rtc_close() {
	/* currently do nothing */
	return 0;
}

int rtc_read() {
	char prev;
	/* avoid new interrupts come in */
	disable_irq(RTC_IRQ_NUM);
	/* select register B and disable NMI */
	outb(REG_B_NMI, RTC_PORT);
	/* read and store current value */
	prev = inb(CMOS_PORT);
	/* set the register to register B again */
	outb(REG_B_NMI, RTC_PORT);
	/* turns off bit 6 of register B to disable PIE */
	outb(prev & ~BIT_SIX, CMOS_PORT);
	/* enable new interrupts come in */
	outb(REG_C, RTC_PORT);
	inb(CMOS_PORT);
	enable_irq(RTC_IRQ_NUM);
	/* set block to 1 */
	block = 1;
	/* wait until new interrupts come in */
	while(1) {
		if (block == 0) {						/* re-enable PIE and return 0 */
			outb(REG_B_NMI, RTC_PORT);
			prev = inb(CMOS_PORT);
			outb(REG_B_NMI, RTC_PORT);
			outb(prev | BIT_SIX, CMOS_PORT);
			return 0;
		}
	}
}

int rtc_write(int rate) {
	char prev;
	char rate_mask;

	if (rate < 2 || rate > 1024)
		return -1;
	if (rate == 2)
		rate_mask = 0x0F;
	else if (rate == 4)
		rate_mask = 0x0E;
	else if (rate == 8)
		rate_mask = 0x0D;
	else if (rate == 16)
		rate_mask = 0x0C;
	else if (rate == 32)
		rate_mask = 0x0B;
	else if (rate == 64)
		rate_mask = 0x0A;
	else if (rate == 128)
		rate_mask = 0x09;
	else if (rate == 256)
		rate_mask = 0x08;
	else if (rate == 512)
		rate_mask = 0x07;
	else if (rate == 1024)
		rate_mask = 0x06;
	else
		return -1;

	disable_irq(RTC_IRQ_NUM);
	outb(REG_A_NMI, RTC_PORT);		// set index to register A, disable NMI
	prev = inb(CMOS_PORT);			// get initial value of register A
	outb(REG_A_NMI, RTC_PORT);		// reset index to A
	outb((prev & 0xF0) | rate_mask, CMOS_PORT); //write only our rate to A.
	enable_irq(RTC_IRQ_NUM);
	return 0;
}
