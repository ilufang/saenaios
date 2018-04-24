/* ata.c - ata driver
 *	vim:ts=4 noexpandtab
 */

#include "ata.h"

#define STAT_ERR_BIT	0x01	///< status bit err
#define STAT_DRQ_BIT	0x08	///< status bit drq
#define STAT_SRV_BIT	0x10	///< status bit srv
#define STAT_DF_BIT		0x20	///< status bit df
#define STAT_RDY_BIT	0x40	///< status bit rdy
#define STAT_BSY_BIT	0x80	///< status bit bsy


#define CMD_RESET			0x4		///< reset command
#define CMD_READ_SEC		0x20	///< sector read command
#define CMD_READ_SEC_EXT	0x24	///< sector read ext command
#define CMD_WRITE_SEC		0x30	///< sector write command
#define CMD_WRITE_SEC_EXT	0x34	///< sector write ext command
#define CMD_CACHE_FLUSH		0xE7	///< cache flush command

static ata_data_t driver_info;		///< local struct to store driver data


/**
 *	ata driver file operations
 */
static file_operations_t ata_fop;

// assembly code reference: https://wiki.osdev.org/ATA_PIO_Mode#CHS_mode

/**
 *	Ata primary irq handler
 *
 *	@note only sends EOI in singletasking mode
 *
 */
void ata_prim_handler(){
	send_eoi(ATA_PRIM_IRQ);
}

/**
 *	Ata secondary irq handler
 *
 *	@note only sends EOI in singletasking mode
 *
 */
void ata_sec_handler(){
	send_eoi(ATA_SEC_IRQ);
	
}

/**
 *	400ns io delay
 *
 *	@param dev: driver data
 *
 */
void io_delay(ata_data_t* dev){
	int32_t reg_offset = dev->io_base_reg;
	// 4 consecutive read to implement 400ns delay
	inb(reg_offset + STATUS_CMD_OFF);
	inb(reg_offset + STATUS_CMD_OFF);
	inb(reg_offset + STATUS_CMD_OFF);
	inb(reg_offset + STATUS_CMD_OFF);
}

/**
 *	Ata software reset
 *
 *	@param dev: driver data
 *
 */
void soft_reset(ata_data_t* dev){
	int32_t reg_offset = dev->io_base_reg;
	outb(CMD_RESET, reg_offset + STATUS_CMD_OFF);
	outb(0, reg_offset + STATUS_CMD_OFF);
	io_delay(dev);
	char stat = inb(reg_offset + STATUS_CMD_OFF);
	// check if BSY clear and RDY set
	while(!(stat & STAT_RDY_BIT) || (stat & STAT_BSY_BIT)){
		stat = inb(reg_offset + STATUS_CMD_OFF);
	}
}

/**
 *	Ata status polling helper function
 *	
 *	Polls status byte until data is ready
 *
 *	@param dev: driver data
 *
 */
int ata_poll_stat(ata_data_t* dev){
	int i = 0;
	uint8_t status;
	
	int32_t reg_offset = dev->io_base_reg;
	
	// ignore error in first 4 status reads
	for( ; i < 4; i++){
		status = inb(reg_offset + STATUS_CMD_OFF);
		// check if bsy is cleared
		if(!status & STAT_BSY_BIT){
			// ready to read data if drq set
			if(status & STAT_DRQ_BIT)
				return 0;
		}
	}
	// loop until bsy clears or error sets
	while(status & STAT_BSY_BIT){
		status = inb(reg_offset + STATUS_CMD_OFF);
		if(status & STAT_DF_BIT || status & STAT_ERR_BIT)
		return -1;
	}
	return 0;
}

/**
 *	28 bit read
 *
 *	Helper function to read a sector in 28 bit lba mode
 *
 *	@param buf: buffer to read into
 *	@param lba: 28 bit mode lba to read from
 *	@param dev: driver data of current device
 *	@return 0 on success, -1 on failure
 */
int ata_read_28(uint32_t lba, uint8_t* buf, ata_data_t* dev){
	
	int32_t reg_offset = dev->io_base_reg;
	int32_t slavebit = dev->slave_bit;
	
	outb(0xE0 | (slavebit << 4) | ((lba >> 24) & 0x0F), reg_offset + DRIVE_HEAD_OFF);
	outb(0x00, reg_offset + ERROR_FEAT_OFF);
	// write sector count to port
	outb(1, reg_offset + SEC_COUNT_OFF);
	// write lba to port
	outb((uint8_t)lba, reg_offset + LBA_LO_OFF);
	outb((uint8_t)(lba>>8), reg_offset + LBA_MID_OFF);
	outb((uint8_t)(lba>>16), reg_offset + LBA_HI_OFF);
	// send read command
	outb(CMD_READ_SEC, reg_offset + STATUS_CMD_OFF);
	
	// poll status byte until data ready
	if(-1 == ata_poll_stat(dev))
		return -1;
	
	int16_t* buffer = (int16_t*)buf;
	// read 512 bytes to buffer
	asm volatile (
			"								\n\
			movl	%1, %%edx				\n\
			movl	$256, %%ecx				\n\
			movl	%0, %%edi				\n\
			rep insw						\n\
			"
			: 
			: "r"(buffer), "r"(reg_offset)
			: "%edx", "%ecx", "%edi"
	);
	// 400ns delay
	io_delay(dev);
	// check status
	uint8_t status = inb(reg_offset + STATUS_CMD_OFF);
	if(status & STAT_DF_BIT || status & STAT_ERR_BIT)
		return -1;
	
	return 0;
}

/**
 *	28 bit write
 *
 *	Helper function to write a sector in 28 bit lba mode
 *
 *	@param buf: buffer to write from
 *	@param lba: 28 bit mode lba to write to
 *	@param dev: driver data of current device
 *	@return 0 on success, -1 on failure
 */
int ata_write_28(uint32_t lba, uint8_t* buf, ata_data_t* dev){
	
	int32_t reg_offset = dev->io_base_reg;
	int32_t slavebit = dev->slave_bit;
	
	outb(0xE0 | (slavebit << 4) | ((lba >> 24) & 0x0F), reg_offset + DRIVE_HEAD_OFF);
	outb(0x00, reg_offset + ERROR_FEAT_OFF);
	// send sector count
	outb(1, reg_offset + SEC_COUNT_OFF);
	// send lba
	outb((uint8_t)lba, reg_offset + LBA_LO_OFF);
	outb((uint8_t)(lba>>8), reg_offset + LBA_MID_OFF);
	outb((uint8_t)(lba>>16), reg_offset + LBA_HI_OFF);
	// send write command
	outb(CMD_WRITE_SEC, reg_offset + STATUS_CMD_OFF);
	
	// poll status byte until data ready
	if(-1 == ata_poll_stat(dev))
		return -1;
	
	// write 512 bytes (256 words)
	int i = 0;
	for( ;i < 256; i++){
		asm volatile (
				"								\n\
				movl	%1, %%edx				\n\
				movl	%0, %%esi				\n\
				outsw							\n\
				"
				: 
				: "r"(buf), "r"(reg_offset)
				: "%edx", "%edi"
		);
		// 400ns delay after each write
		io_delay(dev);
		buf += 2;
	}
	
	// cache flush
	outb(CMD_CACHE_FLUSH, reg_offset + STATUS_CMD_OFF);
	// check status byte for error
	uint8_t status = inb(reg_offset + STATUS_CMD_OFF);
	if(status & STAT_DF_BIT || status & STAT_ERR_BIT)
		return -1;
	
	return 0;
}


int ata_read_st(int32_t sectorcount, uint8_t* buf, int32_t lba, ata_data_t* dev){
	int32_t reg_offset = dev->io_base_reg;
	int read_count = 0;
	// verify sector count
	if(sectorcount<0){
		soft_reset(dev);
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
	//uint8_t sec_len = inb(reg_offset + SEC_COUNT_OFF);
	
	uint8_t	status = inb(reg_offset + STATUS_CMD_OFF);
	if(status | STAT_BSY_BIT || status & STAT_DRQ_BIT){
		soft_reset(dev);
	}
	
	// convert relative lba to absolute lba
	int abs_lba = dev->stLBA + lba;
	int i = 0;
	/*
	if(ebp > 0xFFFFFFF || lba > 0xFFFFFFF ||ebp + lba > 0xFFFFFFF){
		for( ; i < sectorcount; i++){
			if(0 != ata_read_48((int32_t)(abs_lba>>32), (int32_t)abs_lba, buf, dev))
				return -1;
			buf += 512;
		}
	}
	else{
		for( ; i < sectorcount; i++){
			if(0 != ata_read_28((int32_t)(abs_lba + i), buf, dev))
				return -1;
			buf += 512;
		}
	}*/	
	// calls single sector read
	for( ; i < sectorcount; i++){
		if(0 != ata_read_28((int32_t)(abs_lba + i), (uint8_t*)buf, dev))
			return -1;
		buf += 512;
		read_count += 512;
	}
	
	return read_count;
}


ssize_t ata_read(file_t* file, uint8_t *buf, size_t count, off_t *offset){
	// convert offset & count to sectors
	int sec_count = count%512 ? count/512 + 1 : count/512;
	return (ssize_t)ata_read_st(sec_count, buf, (int32_t)((*offset)/512), &driver_info);
}

int ata_write_st(int32_t sectorcount, uint8_t* buf, int32_t lba, ata_data_t* dev){
	int32_t reg_offset = dev->io_base_reg;
	int write_count = 0;
	// verify sector count
	if(sectorcount<0){
		soft_reset(dev);
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
	//uint8_t sec_len = inb(reg_offset + SEC_COUNT_OFF);
	
	uint8_t	status = inb(reg_offset + STATUS_CMD_OFF);
	if(status | STAT_BSY_BIT || status & STAT_DRQ_BIT){
		soft_reset(dev);
	}
	
	// convert relative lba to absolute lba
	int abs_lba =  dev->stLBA + lba;
	int i = 0;
	/*
	if(ebp > 0xFFFFFFF || lba > 0xFFFFFFF ||ebp + lba > 0xFFFFFFF){
		for( ; i < sectorcount; i++){
			if(0 != ata_write_48((int32_t)(abs_lba>>32), (int32_t)abs_lba,buf, dev))
				return -1;
			buf += 512;
		}
	}
	else{
		for( ; i < sectorcount; i++){
			if(0 != ata_write_28((int32_t)(abs_lba + i), buf, dev))
				return -1;
			buf += 512;
		}
	}*/	
	// calls single sector write
	for( ; i < sectorcount; i++){
		if(0 != ata_write_28((int32_t)(abs_lba + i), (uint8_t*)buf, dev))
			return -1;
		buf += 512;
		write_count += 512;
	}
	
	return write_count;
}


ssize_t ata_write(file_t* file, uint8_t *buf, size_t count, off_t *offset){
	int sec_count = count%512 ? count/512 + 1 : count/512;
	return (ssize_t)ata_write_st(sec_count, buf, (int32_t)((*offset)/512), &driver_info);
}

void set_driver_data(ata_data_t* dev_info){
	// initialize driver data
	driver_info.slave_bit = dev_info->slave_bit;
	driver_info.io_base_reg =dev_info->io_base_reg;
	driver_info.prt_size = dev_info->prt_size;
	driver_info.stLBA = dev_info->stLBA;
}

void ata_init(){
	idt_addEventListener(ATA_PRIM_IRQ, &ata_prim_handler);
	idt_addEventListener(ATA_SEC_IRQ, &ata_sec_handler);
	// initialize device info to default(???) settings
	driver_info.slave_bit = 0;
	driver_info.io_base_reg = 0x1F0;
	driver_info.prt_size = 8388608;
	driver_info.stLBA = 0;
}

int32_t ata_open(inode_t* inode, file_t* file){
	ata_init();
	return 1;
}

int32_t ata_close(inode_t* inode, file_t* file){
	return 1;
}

int ata_driver_register(){
	// fill file operation table
	ata_fop.open = &ata_open;
	ata_fop.release = &ata_close;
	ata_fop.read = &ata_read;
	ata_fop.write = &ata_write;
	ata_fop.readdir = NULL;
	return (devfs_register_driver("hda", &ata_fop));
}
