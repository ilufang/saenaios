/* ata.c - ata driver
 *	vim:ts=4 noexpandtab
 */

#include "ata.h"

#define STAT_ERR_BIT	0x01
#define STAT_DRQ_BIT	0x08
#define STAT_SRV_BIT	0x10
#define STAT_DF_BIT		0x20
#define STAT_RDY_BIT	0x40
#define STAT_BSY_BIT	0x80


#define CMD_RESET			0x4
#define CMD_READ_SEC		0x20
#define CMD_READ_SEC_EXT	0x24
#define CMD_WRITE_SEC		0x30
#define CMD_WRITE_SEC_EXT	0x34
#define CMD_CACHE_FLUSH		0xE7

// assembly code reference: https://wiki.osdev.org/ATA_PIO_Mode#CHS_mode

static char slave_bit;

// only need to send eoi in singletasking mode
void ata_prim_handler(){
	send_eoi(ATA_PRIM_IRQ);
}

void ata_sec_handler(){
	send_eoi(KBD_SEC_IRQ);
	
}

void io_delay(){
	// 4 consecutive read to implement 400ns delay
	inb(PRIM_DATA_REG + STATUS_CMD_OFF);
	inb(PRIM_DATA_REG + STATUS_CMD_OFF);
	inb(PRIM_DATA_REG + STATUS_CMD_OFF);
	inb(PRIM_DATA_REG + STATUS_CMD_OFF);
}

void soft_reset(){
	outb(CMD_RESET, PRIM_DATA_REG + STATUS_CMD_OFF);
	outb(0, PRIM_DATA_REG + STATUS_CMD_OFF);
	io_delay;
	char stat = inb(PRIM_DATA_REG + STATUS_CMD_OFF);
	// check if BSY clear and RDY set
	while(stat & STAT_RDY_BIT & STAT_BSY_BIT != STAT_RDY_BIT){
		stat = inb(PRIM_DATA_REG + STATUS_CMD_OFF);
	}
}
// singletasking ata read
int ata_read_st(uint8_t sectorcount, uint16_t* buf, int32_t lba){
	if(sectorcount<0){
		soft_reset();
		if(sectorcount < 0){
			sectorcount = 0;
			return 0;
		}
	}
	// read > 2GB
	if(sectorcount > 0x3FFFFF){
		return -1;
	}
	
	// TODO: verify partition size & sector len
	uint8_t sec_len = inb(PRIM_DATA_REG + SEC_COUNT_OFF);
	
	uint8_t	status = inb(PRIM_DATA_REG + STATUS_CMD_OFF);
	if(status | STAT_BSY_BIT || status & STAT_DRQ_BIT){
		soft_reset();
	}
	int32_t ebp;
	asm volatile (
			"								\n\
			movl	%%ebp, %%eax			\n\
			"
			: "=a"(ebp)
			:
	);
	int abs_lba = ebp + sectorcount;
	if(ebp > 0xFFFFFFF || sectorcount > 0xFFFFFFF ||ebp + sectorcount > 0xFFFFFFF){
		while(0 != ata_read_48((int32_t)(abs_lba>>32), (int32_t)abs_lba, sectorcount, buf));
	}
	else{
		while(0 != ata_read_28((int32_t)abs_lba, sectorcount, buf));
	}	
	
	return 0;
}

int ata_read_28(uint32_t lba, uint8_t sectorcount, uint16_t* buf){
	outb(0xE0 | (slavebit << 4) | ((lba >> 24) & 0x0F), PRIM_DATA_REG + DRIVE_HEAD_OFF);
	outb(0x00, PRIM_DATA_REG + ERROR_FEAT_OFF);
	
	outb(sectorcount, PRIM_DATA_REG + SEC_COUNT_OFF);
	outb((uint8_t)lba, PRIM_DATA_REG + LBA_LO_OFF);
	outb((uint8_t)(lba>>8), PRIM_DATA_REG + LBA_MID_OFF);
	outb((uint8_t)(lba>>16), PRIM_DATA_REG + LBA_HI_OFF);
	
	outb(CMD_READ_SEC, PRIM_DATA_REG + STATUS_CMD_OFF);
	
	int read_count = 4;
	uint8_t status;
	while(1){
		status = inb(PRIM_DATA_REG + STATUS_CMD_OFF);
		if(read_count > 0){
			if(status & STAT_BSY_BIT){
				read_count--;
				continue;
			}
			else if(status & STAT_DRQ_BIT){
				break;
			}
		}else{
			// loop until bsy cleas
			while(status & STAT_BSY_BIT)
				status = inb(PRIM_DATA_REG + STATUS_CMD_OFF);
			if(status & STAT_DF_BIT || status & STAT_ERR_BIT)
				return -1;
		}
	}
	
	asm volatile (
			"								\n\
			movl	%1, %%edx				\n\
			movl	$256, %%ecx				\n\
			movl	%0, edi					\n\
			rep insw						\n\
			"
			: 
			: "r"(buf), "r"(PRIM_DATA_REG)
			: "%edx", "%ecx", "%edi"
	);
	
	io_delay();
	
	status = inb(PRIM_DATA_REG + STATUS_CMD_OFF);
	if(status & STAT_DF_BIT || status & STAT_ERR_BIT)
		return -1;
	
	sectorcount--;
	return sectorcount;
}

int ata_read_48(uint32_t lba_hi, uint32_t lba_lo, uint16_t sectorcount, uint16_t* buf){
	outb(0x40 | (slavebit << 4) | ((lba >> 24) & 0x0F), PRIM_DATA_REG + DRIVE_HEAD_OFF);
	outb(0x00, PRIM_DATA_REG + ERROR_FEAT_OFF);
	
	outb((uint8_t)(sectorcount >> 8), PRIM_DATA_REG + SEC_COUNT_OFF);
	outb((uint8_t)lba_hi, PRIM_DATA_REG + LBA_LO_OFF);
	outb((uint8_t)(lba_hi>>8), PRIM_DATA_REG + LBA_MID_OFF);
	outb((uint8_t)(lba_hi>>16), PRIM_DATA_REG + LBA_HI_OFF);
	
	
	outb((uint8_t)(sectorcount), PRIM_DATA_REG + SEC_COUNT_OFF);
	outb((uint8_t)lba_lo, PRIM_DATA_REG + LBA_LO_OFF);
	outb((uint8_t)(lba_lo>>8), PRIM_DATA_REG + LBA_MID_OFF);
	outb((uint8_t)(lba_lo>>16), PRIM_DATA_REG + LBA_HI_OFF);
	
	outb(CMD_READ_SEC_EXT, PRIM_DATA_REG + STATUS_CMD_OFF);
	
	int read_count = 4;
	uint8_t status;
	
	while(1){
		status = inb(PRIM_DATA_REG + STATUS_CMD_OFF);
		if(read_count > 0){
			if(status & STAT_BSY_BIT){
				read_count--;
				continue;
			}
			else if(status & STAT_DRQ_BIT){
				break;
			}
		}else{
			// loop until bsy clears
			while(status & STAT_BSY_BIT)
				status = inb(PRIM_DATA_REG + STATUS_CMD_OFF);
			if(status & STAT_DF_BIT || status & STAT_ERR_BIT)
				return -1;
		}
	}
	// TODO: checking sector count
	asm volatile (
			"								\n\
			movl	%1, %%edx				\n\
			movl	$256, %%ecx				\n\
			movl	%0, edi					\n\
			rep insw						\n\
			"
			: 
			: "r"(buf), "r"(PRIM_DATA_REG)
			: "%edx", "%ecx", "%edi"
	);
	
	io_delay();
	
	status = inb(PRIM_DATA_REG + STATUS_CMD_OFF);
	if(status & STAT_DF_BIT || status & STAT_ERR_BIT)
		return -1;
	sectorcount--;
	return sectorcount;
}


int ata_write_28(uint32_t lba, uint8_t sectorcount, uint16_t* buf){
	outb(0xE0 | (slavebit << 4) | ((lba >> 24) & 0x0F), PRIM_DATA_REG + DRIVE_HEAD_OFF);
	outb(0x00, PRIM_DATA_REG + ERROR_FEAT_OFF);
	
	outb(sectorcount, PRIM_DATA_REG + SEC_COUNT_OFF);
	outb((uint8_t)lba, PRIM_DATA_REG + LBA_LO_OFF);
	outb((uint8_t)(lba>>8), PRIM_DATA_REG + LBA_MID_OFF);
	outb((uint8_t)(lba>>16), PRIM_DATA_REG + LBA_HI_OFF);
	
	outb(CMD_WRITE_SEC, PRIM_DATA_REG + STATUS_CMD_OFF);
	
	int read_count = 4;
	uint8_t status;
	while(1){
		status = inb(PRIM_DATA_REG + STATUS_CMD_OFF);
		if(read_count > 0){
			if(status & STAT_BSY_BIT){
				read_count--;
				continue;
			}
			else if(status & STAT_DRQ_BIT){
				break;
			}
		}else{
			// loop until bsy clears
			while(status & STAT_BSY_BIT)
				status = inb(PRIM_DATA_REG + STATUS_CMD_OFF);
			if(status & STAT_DF_BIT || status & STAT_ERR_BIT)
				return -1;
		}
	}
	// TODO: DO NOT use rep outsw, need delay and cache flush
	int i = 0;
	for( ;i < 256; i++){
		
		asm volatile (
				"								\n\
				movl	%1, %%edx				\n\
				movl	%0, edi					\n\
				insw							\n\
				"
				: 
				: "r"(buf), "r"(PRIM_DATA_REG)
				: "%edx", "%ecx", "%edi"
		);
		io_delay();
		buf += 16;
	}
	
	// cache flush
	outb(CMD_CACHE_FLUSH, PRIM_DATA_REG + STATUS_CMD_OFF);
	
	status = inb(PRIM_DATA_REG + STATUS_CMD_OFF);
	if(status & STAT_DF_BIT || status & STAT_ERR_BIT)
		return -1;
	
	return 0;
}

int ata_write_48(uint32_t lba_hi, uint32_t lba_lo, uint16_t sectorcount, uint16_t* buf){
	outb(0x40 | (slavebit << 4) | ((lba >> 24) & 0x0F), PRIM_DATA_REG + DRIVE_HEAD_OFF);
	outb(0x00, PRIM_DATA_REG + ERROR_FEAT_OFF);
	
	outb((uint8_t)(sectorcount >> 8), PRIM_DATA_REG + SEC_COUNT_OFF);
	outb((uint8_t)lba_hi, PRIM_DATA_REG + LBA_LO_OFF);
	outb((uint8_t)(lba_hi>>8), PRIM_DATA_REG + LBA_MID_OFF);
	outb((uint8_t)(lba_hi>>16), PRIM_DATA_REG + LBA_HI_OFF);
	
	
	outb((uint8_t)(sectorcount), PRIM_DATA_REG + SEC_COUNT_OFF);
	outb((uint8_t)lba_lo, PRIM_DATA_REG + LBA_LO_OFF);
	outb((uint8_t)(lba_lo>>8), PRIM_DATA_REG + LBA_MID_OFF);
	outb((uint8_t)(lba_lo>>16), PRIM_DATA_REG + LBA_HI_OFF);
	
	outb(CMD_WRITE_SEC_EXT, PRIM_DATA_REG + STATUS_CMD_OFF);
	
	int read_count = 4;
	uint8_t status;
	
	while(1){
		status = inb(PRIM_DATA_REG + STATUS_CMD_OFF);
		if(read_count > 0){
			if(status & STAT_BSY_BIT){
				read_count--;
				continue;
			}
			else if(status & STAT_DRQ_BIT){
				break;
			}
		}else{
			// loop until bsy cleans
			while(status & STAT_BSY_BIT)
				status = inb(PRIM_DATA_REG + STATUS_CMD_OFF);
			if(status & STAT_DF_BIT || status & STAT_ERR_BIT)
				return -1;
		}
	}
	
	// TODO: DO NOT use rep outsw, need delay and cache flush
	int i = 0;
	for( ;i < 256; i++){
		
		asm volatile (
				"								\n\
				movl	%1, %%edx				\n\
				movl	%0, edi					\n\
				insw							\n\
				"
				: 
				: "r"(buf), "r"(PRIM_DATA_REG)
				: "%edx", "%ecx", "%edi"
		);
		io_delay();
		buf += 16;
	}
	
	// cache flush
	outb(CMD_CACHE_FLUSH, PRIM_DATA_REG + STATUS_CMD_OFF);
	
	
	status = inb(PRIM_DATA_REG + STATUS_CMD_OFF);
	if(status & STAT_DF_BIT || status & STAT_ERR_BIT)
		return -1;
	
	return 0;
}

void ata_init(){
	idt_addEventListener(ATA_PRIM_IRQ, &ata_prim_handler);
	idt_addEventListener(ATA_PRIM_IRQ, &ata_sec_handler);
}
