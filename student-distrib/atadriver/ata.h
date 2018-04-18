/**
 *	@file ata.h
 *
 *	Header file for ata driver
 *
 *	vim:ts=4 noexpandtab
 */

#ifndef _ATA_H
#define _ATA_H

#include "../types.h"
#include "../i8259.h"
#include "../lib.h"

#include "../fs/vfs.h"
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



#endif
