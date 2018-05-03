/*
 * Copyright (c) 2013 Grzegorz Kostka (kostka.grzegorz@gmail.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ext4_lwext4_include.h"
#include "../fs/vfs.h"
#include "../fs/file_lookup.h"
#include "../atadriver/ata.h"

#define _FILE_OFFSET_BITS 64

/**@brief   Default filename.*/
static pathname_t fname = "/dev/hda";

/**@brief   Image block size.*/
#define EXT4_FILEDEV_BSIZE 512

/**@brief   Image file descriptor.*/
static file_t* dev_file;
static inode_t* dev_file_inode;

#define DROP_LINUXCACHE_BUFFERS 0

/**********************BLOCKDEV INTERFACE**************************************/
static int filedev_open(struct ext4_blockdev *bdev);
static int filedev_bread(struct ext4_blockdev *bdev, void *buf, uint64_t blk_id,
                         uint32_t blk_cnt);
static int filedev_bwrite(struct ext4_blockdev *bdev, const void *buf,
                          uint64_t blk_id, uint32_t blk_cnt);
static int filedev_close(struct ext4_blockdev *bdev);

/******************************************************************************/
EXT4_BLOCKDEV_STATIC_INSTANCE(_filedev, EXT4_FILEDEV_BSIZE, 0, filedev_open,
                              filedev_bread, filedev_bwrite, filedev_close, NULL, NULL);

/******************************************************************************/
static int filedev_open(struct ext4_blockdev *bdev)
{
    inode_t* dev_file_inode = file_lookup(fname);
    if (!dev_file_inode)
        return ENOENT;

    dev_file = vfs_open_file(dev_file_inode, 1);

    if (!dev_file)
        return EIO;

    dev_file->pos = ata_identify();

    _filedev.bdif->ph_bcnt = dev_file->pos / _filedev.bdif->ph_bsize;
    _filedev.part_size = EXT4_FILEDEV_BSIZE * (_filedev.bdif->ph_bcnt);

    return EOK;
}

/******************************************************************************/

static int filedev_bread(struct ext4_blockdev *bdev, void *buf, uint64_t blk_id,
                         uint32_t blk_cnt)
{
    off_t temp_offset = 1;
/*    if ((*dev_file->f_op->llseek)(dev_file, blk_id * bdev->bdif->ph_bsize, SEEK_SET))
        return EIO;*/
    dev_file->pos = blk_id * bdev->bdif->ph_bsize + 32256;

    if (!(*dev_file->f_op->read)(dev_file, buf, bdev->bdif->ph_bsize * blk_cnt, &dev_file->pos))
        return EIO;

    return EOK;
}

static void drop_cache(void)
{
#if defined(__linux__) && DROP_LINUXCACHE_BUFFERS
    int fd;
    char *data = "3";

    sync();
    fd = open("/proc/sys/vm/drop_caches", O_WRONLY);
    write(fd, data, sizeof(char));
    close(fd);
#endif
}

/******************************************************************************/
static int filedev_bwrite(struct ext4_blockdev *bdev, const void *buf,
                          uint64_t blk_id, uint32_t blk_cnt)
{
    off_t temp_offset = 1;

/*    if ((*dev_file->f_op->llseek)(dev_file, blk_id * bdev->bdif->ph_bsize, SEEK_SET))
        return EIO;*/
    dev_file->pos = blk_id * bdev->bdif->ph_bsize + 32256;

    if (!(*dev_file->f_op->write)(dev_file, (uint8_t*)buf, bdev->bdif->ph_bsize * blk_cnt, &dev_file->pos))
        return EIO;

    drop_cache();
    return EOK;
}
/******************************************************************************/
static int filedev_close(struct ext4_blockdev *bdev)
{
    (*dev_file->f_op->release)(dev_file_inode, dev_file);
    return EOK;
}

/******************************************************************************/
struct ext4_blockdev *ext4_filedev_get(void) { return &_filedev; }
/******************************************************************************/
void ext4_filedev_filename(const char *n) { return; }

/******************************************************************************/
