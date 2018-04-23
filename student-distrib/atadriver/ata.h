/**
 *	@file ata.h
 *
 *	Header file for ata driver
 *
 *	vim:ts=4 noexpandtab
 */

#ifndef _ATA_H
#define _ATA_H


#include "../boot/idt.h"
#include "../types.h"
#include "../i8259.h"
#include "../lib.h"

#include "../fs/vfs.h"
#include "../fs/fs_devfs.h"
#include "../errno.h"
#include "../fs/fstab.h"
#include "../../libc/include/dirent.h"

#define ATA_PRIM_IRQ	14	///< primary bus irq num
#define ATA_SEC_IRQ		15	///< secondary bus irq num

#define PRIM_DATA_REG	0x1F0	///< primary IO port: 0x1F0 - 0x1F7
#define ERROR_FEAT_OFF	0x1	///< feature/error register
#define SEC_COUNT_OFF	0x2	///< sector count
#define SECTOR_NUM_OFF	0x3	///< sector number
#define LBA_LO_OFF		0x3	///< lba_lo
#define CYLIND_LOW_OFF	0x4	///< cylinder low
#define LBA_MID_OFF		0x4	///< lba_mid
#define CYLIND_HI_OFF	0x5	///< cylinder high
#define LBA_HI_OFF		0x5	///< lba_hi
#define DRIVE_HEAD_OFF	0x6	///< drive/head
#define STATUS_CMD_OFF	0x7	///< status/command

#define SEC_DATA_REG	0x170	///< secondary IO port: 0x170 - 0x177

#define PRIM_CTL_REG	0x3F6	
#define SEC_CTL_REG		0x376
#define ERROR_REG		0x1F1

typedef struct s_ata {
	int32_t slave_bit;		///< master/slave
	int32_t io_base_reg;	///< primary/secondary
	int32_t stLBA;			///< lba offset
	int32_t prt_size;		///< partition size
} ata_data_t;

//lseek
void ata_init();

void set_driver_data(ata_data_t* usr_data);

int ata_read_st(int32_t sectorcount, uint8_t* buf, int32_t lba, ata_data_t* dev);

int ata_write_st(int32_t sectorcount, uint8_t* buf, int32_t lba, ata_data_t* dev);

ssize_t ata_write(file_t* file, uint8_t *buf, size_t count, off_t *offset);

ssize_t ata_read(file_t* file, uint8_t *buf, size_t count, off_t *offset);

int32_t ata_open(inode_t* inode, file_t* file);

int32_t ata_close(inode_t* inode, file_t* file);

int ata_driver_register();

#endif
