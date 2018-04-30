/* rtc.c - linux rtc driver
 * vim:ts=4 noexpandtab
 */
#include "rtc.h"
#include "boot/idt.h"
#include "lib.h"
#include "errno.h"

#include "proc/scheduler.h"
#include "proc/task.h"
#include "proc/signal.h"

static volatile int rtc_count_prev = 0;
static volatile int rtc_count = 1;
static int rtc_openfile = -1;
volatile int previous_enter = -1;

#define RTC_MAX_FREQ 	1024	/* max user frequency is 1024 Hz */
#define RTC_IS_OPEN 	0x01	/* means rtc is opened in a file */
#define RTC_MAX_OPEN 	256		/* max open file of rtc */
#define ALRM_MAX_TIMER 	999999

static rtc_file_t rtc_file_table[RTC_MAX_OPEN];
pid_t rtc_pid_waiting[RTC_MAX_OPEN];
static file_operations_t rtc_out_op;

int rtc_out_driver_register() {

	int i; //iterator

	rtc_out_op.open = &rtc_open;
	rtc_out_op.release = &rtc_close;
	rtc_out_op.read = &rtc_read;
	rtc_out_op.write = &rtc_write;
	rtc_out_op.readdir = NULL;

	for (i = 0; i < RTC_MAX_OPEN; i++) {
        rtc_file_table[i].rtc_pid = -1;
		rtc_file_table[i].rtc_status = 0;
		rtc_file_table[i].rtc_freq = 0;
        rtc_file_table[i].rtc_sleep = -1;
        rtc_file_table[i].timer.it_value = 0;
        rtc_file_table[i].timer.it_interval = 0;
        rtc_pid_waiting[i] = -1;
        // rtc_pid_waiting[RTC_MAX_OPEN] = -1;
	}

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
	rtc_setrate(0x06);
	idt_addEventListener(RTC_IRQ_NUM, &rtc_handler);
}

void rtc_handler(){
	/* sends eoi */
	int iter; // iterator
    rtc_count_prev = rtc_count;
    // Update count and alrm timer
    for(iter = 0; iter < rtc_openfile+1; iter++) {
	    if (rtc_count != rtc_count_prev && rtc_count % RTC_MAX_FREQ == 0) {
	    	if (rtc_file_table[iter].timer.it_value < 0) {
	    		rtc_file_table[iter].timer.it_value = 0;
	    		rtc_file_table[iter].timer.it_interval = 0;
	    		break;
	    	}

	    	if(rtc_file_table[iter].timer.it_value-- == 0) {
	    		rtc_file_table[iter].timer.it_value = rtc_file_table[iter].timer.it_interval;
	    		rtc_file_table[iter].timer.it_interval = 0;
	    		syscall_kill(rtc_file_table[iter].rtc_pid, SIGALRM, 0);
	    	}
	    }
	}
	rtc_count++;

    // if ((rtc_count != rtc_count_prev) &&
    //     (rtc_count & (rtc_file_table[rtc_openfile].rtc_freq-1)) == 0) {
        
    //     syscall_kill(rtc_file_table[rtc_openfile].rtc_pid, SIGIO, 0);
    //     // rtc_pid_waiting[rtc_openfile] = 0;
    //     rtc_file_table[rtc_openfile].rtc_sleep = 0;

    // }

    if ((rtc_count != rtc_count_prev)) {
    	for (iter = 0; iter < rtc_openfile+1; iter++) {
    		if ((rtc_file_table[iter].rtc_freq != 0) &&
    			((rtc_count & (rtc_file_table[iter].rtc_freq-1)) == 0) &&
    			(rtc_file_table[iter].rtc_sleep == 1)) {
    			syscall_kill(rtc_file_table[iter].rtc_pid, SIGIO, 0);
    			rtc_file_table[iter].rtc_sleep = 0;
    		}
    	}
    }

	send_eoi(RTC_IRQ_NUM);
	// rtc_read();
	// test_interrupts();
	/* reads from register C so that the interrupt will happen again */
	outb(REG_C, RTC_PORT);
	inb(CMOS_PORT);
	if (scheduler_on_flag) {
		scheduler_event();
	}
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
	// if (rtc_status & RTC_IS_OPEN)
	// 	return 0;
	/* initialize private data */
	// rtc_status |= RTC_IS_OPEN;
	// rtc_count = 1;
	// rtc_freq = 512;
	// rtc_setrate(0x06);

	rtc_openfile++;
	if (rtc_file_table[rtc_openfile].rtc_status & RTC_IS_OPEN)
		return 0;
	rtc_file_table[rtc_openfile].rtc_status |= RTC_IS_OPEN;
	rtc_file_table[rtc_openfile].rtc_freq = 512;
	rtc_file_table[rtc_openfile].rtc_pid = task_current_pid();
	rtc_file_table[rtc_openfile].rtc_sleep = -1;
	rtc_file_table[rtc_openfile].timer.it_interval = 0;
	rtc_file_table[rtc_openfile].timer.it_value = 0;
	file->private_data = rtc_openfile;
	rtc_count = 1;

	return 0;
}

int rtc_close(inode_t* inode, file_t* file) {
	/* currently do nothing */
	// rtc_status &= ~RTC_IS_OPEN;
	// rtc_status = 0;
	// rtc_freq = 0;
	// rtc_count = 1;
	int i;
	i = file->private_data;
	rtc_file_table[i].rtc_status &= ~RTC_IS_OPEN;
	rtc_file_table[i].rtc_freq = 0;
	rtc_file_table[i].rtc_pid = -1;
	rtc_file_table[i].rtc_sleep = -1;
	rtc_file_table[i].timer.it_interval = 0;
	rtc_file_table[i].timer.it_value = 0;
	rtc_count = 1;
	// rtc_openfile--;
	return 0;
}

ssize_t rtc_read(file_t* file, uint8_t* buf, size_t count, off_t* offset) {
    
    // Now have to call write before calling read
    int i;

    i = file->private_data;

    if (rtc_file_table[i].rtc_status == 0) {
        return -EINVAL;
    }
    
    task_sigact_t sa;
    sigset_t ss;
    pid_t cur_pid = task_current_pid();
    
/*
    int v_rtc_status;
    int v_rtc_freq;
    v_rtc_status = rtc_file_table[*buf].rtc_status;
    v_rtc_freq = rtc_file_table[*buf].rtc_freq;
*/
    // Code in Keyboard, needs to be change, TODO
    
    if (rtc_file_table[i].rtc_sleep < 0) {
        
        // set process to sleep until SIGIO
        
        rtc_pid_waiting[i] = cur_pid;
        
        sa.handler = SIG_IGN;
        sigemptyset(&(sa.mask)); // unmask
        
        sa.flags = SA_RESTART;

        syscall_sigaction(SIGIO, (int)&sa, 0);
        rtc_file_table[i].rtc_sleep = 1;
        sigemptyset(&ss);
        syscall_sigsuspend((int)&ss, NULL, 0);
        return 0;
        
    }
    
    if (rtc_file_table[i].rtc_sleep == 0) {
        rtc_file_table[i].rtc_sleep = -1;
        return 0;
    } 

    else {
        return 0;
    }

}

ssize_t rtc_write(file_t* file, uint8_t* buf, size_t count, off_t* offset) {

	int i = file->private_data;

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
	rtc_file_table[i].rtc_freq = RTC_MAX_FREQ / freq;

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

int getitimer(struct itimerval *value) {
	/* sanity check */
	if (value == NULL) {
		return -EINVAL;
	}

	if (rtc_file_table[rtc_openfile].timer.it_value == 0) {
		value->it_value = 0;
		value->it_interval = 0;
		printf("TIMER NOT SET!");
		return 0;
	} else {
		value->it_value = rtc_file_table[rtc_openfile].timer.it_value;
		value->it_interval = rtc_file_table[rtc_openfile].timer.it_interval;
		return 0;
	}
}

int setitimer(struct itimerval *value, struct itimerval *old_value) {

	if (value == NULL || old_value == NULL) {
		return -EINVAL;
	}

	if (value->it_interval > ALRM_MAX_TIMER || 
		value->it_value  > ALRM_MAX_TIMER) {
		return -EINVAL;
	}

	old_value->it_interval = rtc_file_table[rtc_openfile].timer.it_interval;
	old_value->it_interval = rtc_file_table[rtc_openfile].timer.it_value;
	rtc_file_table[rtc_openfile].timer.it_interval = value->it_interval;
	rtc_file_table[rtc_openfile].timer.it_value = value->it_value;

	return 0;

}

int nanosleep(struct itimerval *requested, struct itimerval *remain) {

	if (requested == NULL || remain == NULL) {
		return -EINVAL;
	}
	if (requested->it_value == 0 || requested->it_value > ALRM_MAX_TIMER) {
		return -EINVAL;
	}
	if (requested->it_interval != 0) {
		return -EINVAL;
	}
	return setitimer(requested, remain);

}

