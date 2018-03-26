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

static file_operations_t rtc_out_op;

int rtc_out_driver_register() {

	rtc_out_op.open = &rtc_open;
	rtc_out_op.release = &rtc_close;
	rtc_out_op.read = &rtc_read;
	rtc_out_op.write = &rtc_write;
	rtc_out_op.readdir = NULL;

	return (devfs_register_driver("rtc", &rtc_out_op));
}

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

int rtc_open(inode_t* inode, file_t* file) {
	/* check if rtc is already opened */
	if (rtc_status & RTC_IS_OPEN)
		return 0;
	/* initialize private data */
	rtc_status |= RTC_IS_OPEN;
	rtc_count = 1;
	rtc_freq = 512;

	return 0;
}

int rtc_close(inode_t* inode, file_t* file) {
	/* currently do nothing */
	rtc_status &= ~RTC_IS_OPEN;
	rtc_status = 0;
	rtc_freq = 0;
	rtc_count = 1;
	return 0;
}

ssize_t rtc_read(file_t* file, uint8_t* buf, size_t count, off_t* offset) {

	if (rtc_status == 0 || rtc_freq == 0)
		return 0;
	while(1) {
		if (rtc_count != rtc_count_prev && (rtc_count & (rtc_freq-1)) == 0) {
			rtc_count_prev = rtc_count;
			return 0;
		}
	}

}

ssize_t rtc_write(file_t* file, uint8_t* buf, size_t count, off_t* offset) {

	if (buf == NULL || count < 4) {
		return -EINVAL;
	}
	/* sanity check */
	int freq = *buf;
	if (!RTC_IS_OPEN)
		return -EINVAL;

	if (freq < 2 || freq > 1024)
		return -EINVAL;

	if (is_power_of_two(freq) == -1)
		return -EINVAL;
	/* set rtc_freq */
	rtc_freq = RTC_MAX_FREQ / freq;

	return 0;

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
