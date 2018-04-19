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

// only need to send eoi in singletasking mode
void ata_prim_handler(){
	send_eoi(ATA_PRIM_IRQ);
}

void ata_sec_handler(){
	send_eoi(KBD_SEC_IRQ);
	
}

void io_delay(ata_data_t* dev){
	int32_t reg_offset = dev->io_base_reg;
	// 4 consecutive read to implement 400ns delay
	inb(reg_offset + STATUS_CMD_OFF);
	inb(reg_offset + STATUS_CMD_OFF);
	inb(reg_offset + STATUS_CMD_OFF);
	inb(reg_offset + STATUS_CMD_OFF);
}

void soft_reset(ata_data_t* dev){
	int32_t reg_offset = dev->io_base_reg;
	outb(CMD_RESET, reg_offset + STATUS_CMD_OFF);
	outb(0, reg_offset + STATUS_CMD_OFF);
	io_delay;
	char stat = inb(reg_offset + STATUS_CMD_OFF);
	// check if BSY clear and RDY set
	while(stat & STAT_RDY_BIT & STAT_BSY_BIT != STAT_RDY_BIT){
		stat = inb(reg_offset + STATUS_CMD_OFF);
	}
}
// singletasking ata read
int ata_read_st(uint8_t sectorcount, uint16_t* buf, int32_t lba, ata_data_t* dev){
	int32_t reg_offset = dev->io_base_reg;
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
	uint8_t sec_len = inb(reg_offset + SEC_COUNT_OFF);
	
	uint8_t	status = inb(reg_offset + STATUS_CMD_OFF);
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
	int abs_lba = ebp + lba;
	int i = 0;
	if(ebp > 0xFFFFFFF || lba > 0xFFFFFFF ||ebp + lba > 0xFFFFFFF){
		for( ; i < sectorcount; i++){
			if(0 != ata_read_48((int32_t)(abs_lba>>32), (int32_t)abs_lba, sectorcount, buf, dev))
				return -1;
			buf += 512;
		}
	}
	else{
		for( ; i < sectorcount; i++){
			if(0 != ata_read_28((int32_t)abs_lba, sectorcount, buf, dev))
				return -1;
			buf += 512;
		}
	}	
	
	return 0;
}

int ata_poll_stat(ata_data_t* dev){
	int i = 0;
	uint8_t status;
	
	int32_t reg_offset = dev->io_base_reg;
	
	// ignore error in first 4 status reads
	for( ; i < 4; i++){
		status = inb(io_base + STATUS_CMD_OFF);
		// check if bsy is cleared
		if(!status & STAT_BSY_BIT){
			// ready to read data if drq set
			if(status & STAT_DRQ_BIT)
				return 0;
		}
	}
	// loop until bsy clears or error sets
	while(status & STAT_BSY_BIT){
		status = inb(io_base + STATUS_CMD_OFF);
		if(status & STAT_DF_BIT || status & STAT_ERR_BIT)
		return -1;
	}
	return 0;
}

int ata_read_28(uint32_t lba, uint8_t* buf, ata_data_t* dev){
	
	int32_t reg_offset = dev->io_base_reg;
	int32_t slavebit = dev->slave_bit;
	
	outb(0xE0 | (slavebit << 4) | ((lba >> 24) & 0x0F), reg_offset + DRIVE_HEAD_OFF);
	outb(0x00, reg_offset + ERROR_FEAT_OFF);
	
	outb(sectorcount, reg_offset + SEC_COUNT_OFF);
	outb((uint8_t)lba, reg_offset + LBA_LO_OFF);
	outb((uint8_t)(lba>>8), reg_offset + LBA_MID_OFF);
	outb((uint8_t)(lba>>16), reg_offset + LBA_HI_OFF);
	
	outb(CMD_READ_SEC, reg_offset + STATUS_CMD_OFF);
	
	if(-1 == ata_poll_stat(dev))
		return -1;
	
	int16_t* buffer = (int16_t*)buf;
	
	asm volatile (
			"								\n\
			movl	%1, %%edx				\n\
			movl	$256, %%ecx				\n\
			movl	%0, edi					\n\
			rep insw						\n\
			"
			: 
			: "r"(buffer), "r"(reg_offset)
			: "%edx", "%ecx", "%edi"
	);
	
	io_delay();
	
	status = inb(reg_offset + STATUS_CMD_OFF);
	if(status & STAT_DF_BIT || status & STAT_ERR_BIT)
		return -1;
	
	return 0;
}

int ata_read_48(uint32_t lba_hi, uint32_t lba_lo, uint16_t* buf, ata_data_t* dev){
	
	
	int32_t reg_offset = dev->io_base_reg;
	int32_t slavebit = dev->slave_bit;
	
	outb(0x40 | (slavebit << 4) | ((lba >> 24) & 0x0F), reg_offset + DRIVE_HEAD_OFF);
	outb(0x00, reg_offset + ERROR_FEAT_OFF);
	
	outb((uint8_t)(sectorcount >> 8), reg_offset + SEC_COUNT_OFF);
	outb((uint8_t)lba_hi, reg_offset + LBA_LO_OFF);
	outb((uint8_t)(lba_hi>>8), reg_offset + LBA_MID_OFF);
	outb((uint8_t)(lba_hi>>16), reg_offset + LBA_HI_OFF);
	
	
	outb((uint8_t)(sectorcount), reg_offset + SEC_COUNT_OFF);
	outb((uint8_t)lba_lo, reg_offset + LBA_LO_OFF);
	outb((uint8_t)(lba_lo>>8), reg_offset + LBA_MID_OFF);
	outb((uint8_t)(lba_lo>>16), reg_offset + LBA_HI_OFF);
	
	outb(CMD_READ_SEC_EXT, reg_offset + STATUS_CMD_OFF);
	
	int read_count = 4;
	uint8_t status;
	
	if(-1 == ata_poll_stat(dev))
		return -1;
	
	int16_t* buffer = (int16_t*)buf;
	
	asm volatile (
			"								\n\
			movl	%1, %%edx				\n\
			movl	$256, %%ecx				\n\
			movl	%0, edi					\n\
			rep insw						\n\
			"
			: 
			: "r"(buffer), "r"(reg_offset)
			: "%edx", "%ecx", "%edi"
	);
	
	io_delay();
	
	status = inb(reg_offset + STATUS_CMD_OFF);
	if(status & STAT_DF_BIT || status & STAT_ERR_BIT)
		return -1;
	
	return 0;
}


int ata_write_28(uint32_t lba, uint16_t* buf, ata_data_t* dev){
	
	int32_t reg_offset = dev->io_base_reg;
	int32_t slavebit = dev->slave_bit;
	
	outb(0xE0 | (slavebit << 4) | ((lba >> 24) & 0x0F), reg_offset + DRIVE_HEAD_OFF);
	outb(0x00, reg_offset + ERROR_FEAT_OFF);
	
	outb(sectorcount, reg_offset + SEC_COUNT_OFF);
	outb((uint8_t)lba, reg_offset + LBA_LO_OFF);
	outb((uint8_t)(lba>>8), reg_offset + LBA_MID_OFF);
	outb((uint8_t)(lba>>16), reg_offset + LBA_HI_OFF);
	
	outb(CMD_WRITE_SEC, reg_offset + STATUS_CMD_OFF);
	
	if(-1 == ata_poll_stat(dev))
		return -1;
	// TODO: DO NOT use rep outsw, need delay and cache flush
	int i = 0;
	for( ;i < 256; i++){
		
		asm volatile (
				"								\n\
				movl	%1, %%edx				\n\
				movl	%0, edi					\n\
				outsw							\n\
				"
				: 
				: "r"(buf), "r"(reg_offset)
				: "%edx", "%ecx", "%edi"
		);
		io_delay();
		buf += 2;
	}
	
	// cache flush
	outb(CMD_CACHE_FLUSH, reg_offset + STATUS_CMD_OFF);
	
	status = inb(reg_offset + STATUS_CMD_OFF);
	if(status & STAT_DF_BIT || status & STAT_ERR_BIT)
		return -1;
	
	return 0;
}

int ata_write_48(uint32_t lba_hi, uint32_t lba_lo, uint16_t* buf, ata_data_t* dev){
	
	int32_t reg_offset = dev->io_base_reg;
	int32_t slavebit = dev->slave_bit;
	
	outb(0x40 | (slavebit << 4) | ((lba >> 24) & 0x0F), reg_offset + DRIVE_HEAD_OFF);
	outb(0x00, reg_offset + ERROR_FEAT_OFF);
	
	outb((uint8_t)(sectorcount >> 8), reg_offset + SEC_COUNT_OFF);
	outb((uint8_t)lba_hi, reg_offset + LBA_LO_OFF);
	outb((uint8_t)(lba_hi>>8), reg_offset + LBA_MID_OFF);
	outb((uint8_t)(lba_hi>>16), reg_offset + LBA_HI_OFF);
	
	
	outb((uint8_t)(sectorcount), reg_offset + SEC_COUNT_OFF);
	outb((uint8_t)lba_lo, reg_offset + LBA_LO_OFF);
	outb((uint8_t)(lba_lo>>8), reg_offset + LBA_MID_OFF);
	outb((uint8_t)(lba_lo>>16), reg_offset + LBA_HI_OFF);
	
	outb(CMD_WRITE_SEC_EXT, reg_offset + STATUS_CMD_OFF);
	
	if(-1 == ata_poll_stat(dev))
		return -1;
	
	// TODO: DO NOT use rep outsw, need delay and cache flush
	int i = 0;
	for( ;i < 256; i++){
		
		asm volatile (
				"								\n\
				movl	%1, %%edx				\n\
				movl	%0, edi					\n\
				outsw							\n\
				"
				: 
				: "r"(buf), "r"(reg_offset)
				: "%edx", "%ecx", "%edi"
		);
		io_delay();
		buf += 2;
	}
	
	// cache flush
	outb(CMD_CACHE_FLUSH, reg_offset + STATUS_CMD_OFF);
	
	
	status = inb(reg_offset + STATUS_CMD_OFF);
	if(status & STAT_DF_BIT || status & STAT_ERR_BIT)
		return -1;
	
	return 0;
}

void ata_init(){
	idt_addEventListener(ATA_PRIM_IRQ, &ata_prim_handler);
	idt_addEventListener(ATA_PRIM_IRQ, &ata_sec_handler);
}
