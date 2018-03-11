/* rtc.c - linux rtc driver
 * vim:ts=4 noexpandtab
 */

#include "rtc.h"
#include "lib.h"

/* 
 * rtc_init 
 * DESCRIPTION: initialize the rtc
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: writes to rtc control register, enables irq8 on PIC
 */
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
    enable_irq(RTC_IRQ_NUM);

}

/* 
 * rtc_handler
 * DESCRIPTION: temporary rtc interrupt handler, 
 * only calls test_interrupte() to make sure that
 * we are able to receive interrupt
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: changes video memory content
 */
void rtc_handler(){
    /* sends eoi */
    send_eoi(RTC_IRQ_NUM);
    /* calls tet function in lib.c */
    test_interrupts();
    /* reads from register C so that the interrupt will happen again */
    outb(REG_C, RTC_PORT);
    inb(CMOS_PORT);
}
