/* rtc.c - linux rtc driver
 * vim:ts=4 noexpandtab
 */
#include "rtc.h"
#include "boot/idt.h"
#include "lib.h"
#include "errno.h"

static int rtc_status = 0;
static volatile int rtc_freq = 0;
static volatile int rtc_count = 1;
static volatile int rtc_count_prev = 0;

#define RTC_MAX_FREQ 	1024	/* max user frequency is 1024 Hz */
#define RTC_IS_OPEN 	0x01	/* means rtc is opened in a file */

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
	rtc_count++;
	send_eoi(RTC_IRQ_NUM);
	// rtc_read();
	// test_interrupts();
	/* reads from register C so that the interrupt will happen again */
	outb(REG_C, RTC_PORT);
	inb(CMOS_PORT);
}

void test_rtc_handler() {
	rtc_count++;
	send_eoi(RTC_IRQ_NUM);
	// test_interrupts();
	outb(REG_C, RTC_PORT);
	inb(CMOS_PORT);
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
	/* check if rtc is already opened */
	if (rtc_status & RTC_IS_OPEN)
		return 0;
	/* initialize private data */
	rtc_status |= RTC_IS_OPEN;
	rtc_count = 1;
	rtc_freq = 512;
	/* avoid new interrupts come in */
	// disable_irq(RTC_IRQ_NUM);
	/* select register A and disbale NMI */
	// outb(REG_A_NMI, RTC_PORT);
	/* read and store current value */
	// char prev = inb(CMOS_PORT);
	/* select register A again */
	// outb(REG_A_NMI, RTC_PORT);
	/* set the 0-3 bits to adjust frequency to 2Hz */
	// outb(((prev & 0xF0) | 0x06), CMOS_PORT);
	/* selects register B */
	// outb(REG_B_NMI, RTC_PORT);
	/* read and store current value */
	// prev = inb(CMOS_PORT);
	/* set the register to register B again */
	// outb(REG_B_NMI, RTC_PORT);
	/* turns on bit 6 of register B to enable PIE */
	// outb(prev | BIT_SIX, CMOS_PORT);
	/* enable new interrupts come in */
	// enable_irq(RTC_IRQ_NUM);
	/* return 0 for successful open */
	return 0;
}

int32_t rtc_close(int32_t fd) {
	/* currently do nothing */
	rtc_status &= ~RTC_IS_OPEN;
	rtc_status = 0;
	rtc_freq = 0;
	rtc_count = 1;
	return 0;
}

int rtc_read() {

	if (rtc_status == 0 || rtc_freq == 0)
		return 0;
	while(1) {
		if (rtc_count != rtc_count_prev && (rtc_count & (rtc_freq-1)) == 0) {
			rtc_count_prev = rtc_count;
			return 0;
		}
	}

}

int rtc_write(int freq) {
	/* sanity check */
	if (!RTC_IS_OPEN)
		return -EINVAL;

	if (freq < 2 || freq > 1024)
		return -EINVAL;

	if (is_power_of_two(freq) == -1)
		return -EINVAL;
	/* set rtc_freq */
	rtc_freq = RTC_MAX_FREQ / freq;

	return 0;

/*
	char prev;
	char rate_mask; 
*/
	int rate = *((int*)(buf));
	if (rate < 2 || rate > 1024)
		return -1;
/*
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
*/
}

int is_power_of_two(int freq) {

	if (freq == 0)
		return 0;
	while (freq != 1) {
		if (freq % 2 != 0)
			return -1;
		freq = freq / 2;
	}
	return 0;
}
