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

#define PRIM_CTL_REG	0x3F6	///< primary control register
#define SEC_CTL_REG		0x376	///< secondary control register
#define ERROR_REG		0x1F1	///< error register

/**
 *	ata driver info struct
 */
typedef struct s_ata {
	int32_t slave_bit;		///< master/slave
	int32_t io_base_reg;	///< primary/secondary
	int32_t stLBA;			///< lba offset
	int32_t prt_size;		///< partition size
} ata_data_t;

/**
 *	Ata initialization function
 */
void ata_init();

/**
 *	Set ata driver data
 *
 *	@param usr_data: user defined driver data
 */
void set_driver_data(ata_data_t* usr_data);

/**
 *	Ata singletasking read
 *
 *	@param sectorcount: number of sector to read
 *	@param buf: buffer to read into
 *	@param lba: 28 bit mode lba to read from
 *	@param dev: driver data of current device
 *	@return number of byte read on success, -1 on failure
 */
int ata_read_st(int32_t sectorcount, uint8_t* buf, int32_t lba, ata_data_t* dev);

/**
 *	Ata singletasking write
 *
 *	@param sectorcount: number of sector to write
 *	@param buf: buffer to write from
 *	@param lba: 28 bit mode lba to write to
 *	@param dev: driver data of current device
 *	@return number of byte wrote on success, -1 on failure
 */
int ata_write_st(int32_t sectorcount, uint8_t* buf, int32_t lba, ata_data_t* dev);

/**
 *	Ata write handler
 *
 *	@param file: the file obj
 *	@param buf: buffer to write from
 *	@param count: number of byte to write
 *	@param offset: offset to start writing
 *	@return number of byte read on success, -1 on failure
 */
ssize_t ata_write(file_t* file, uint8_t *buf, size_t count, off_t *offset);

/**
 *	Ata read handler
 *
 *	@param file: the file obj
 *	@param buf: buffer to read into
 *	@param count: number of byte to read
 *	@param offset: offset to start reading
 *	@return number of byte read on success, -1 on failure
 */
ssize_t ata_read(file_t* file, uint8_t *buf, size_t count, off_t *offset);

/**
 *	Ata open function
 *
 *	Initialize local driver data
 *
 *	@param file: file object
 *	@param inode: inode of the file to close
 *	@return 0 on success
 *
 */
int32_t ata_open(inode_t* inode, file_t* file);

/**
 *	Ata close function
 *
 *	@param file: file object
 *	@param inode: inode of the file to close
 *	@return 0 on success
 *
 */
int32_t ata_close(inode_t* inode, file_t* file);

/**
 *	Ata driver registration
 *
 *	Register driver in devfs
 *
 *	@return 0 on success
 *
 */
int ata_driver_register();

#endif
