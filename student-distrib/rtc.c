/* rtc.c - linux rtc driver
 * vim:ts=4 noexpandtab
 */

#include "rtc.h"
#include "lib.h"

/* Initialize the rtc */
void rtc_init(){
                                            // disable interrupts
    outb(REG_B_NMI, RTC_PORT);		            // select register B, and disable NMI
    unsigned char prev = inb(CMOS_PORT);    // read the current value of register B
    outb(REG_B_NMI, RTC_PORT);	                // set the index again
    outb(prev | BIT_SIX, CMOS_PORT);	    // turns on bit 6 of register B
    
    enable_irq(RTC_IRQ_NUM);

}

/* interrupt handler(for now) */
void rtc_handler(){
    send_eoi(RTC_IRQ_NUM);
    test_interrupts();
    outb(REG_C, RTC_PORT);	// select register C
    inb(CMOS_PORT);
}
