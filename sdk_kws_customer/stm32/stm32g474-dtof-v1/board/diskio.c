/*
 * diskio.c
 *
 *  Created on: 2024/9/11
 *      Author: liuzihao
 */

#include "fatfs/diskio.h"
#include "board.h"
#include "base/inc/mos_platform.h"
#include "ffconf.h"

#if defined ( __GNUC__ )
#ifndef __weak
#define __weak __attribute__((weak))
#endif
#endif

#define INNER_FLASH 0

static int innerflash_fd = -1;

/**
  * @brief  Gets Disk Status
  * @param  pdrv: Physical drive number (0..)
  * @retval DSTATUS: Operation status
  */
DSTATUS disk_status(BYTE pdrv /* Physical drive number to identify the drive */
) {
    return RES_OK;
}

/**
  * @brief  Initializes a Drive
  * @param  pdrv: Physical drive number (0..)
  * @retval DSTATUS: Operation status
  */
DSTATUS disk_initialize(
    BYTE pdrv /* Physical drive nmuber to identify the drive */
) {
    DSTATUS stat = STA_NOINIT;
    int result;

    switch (pdrv) {
        case INNER_FLASH:
            // TODO:@liuzihao TBD
            // innerflash_fd = dtof_device_open(DEVICE_NAME_FLASH, MOS_RDWR);
            if (innerflash_fd >= 0) {
                stat = 0;
            }
            break;
    }
    return stat;
}

/**
  * @brief  Reads Sector(s)
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT disk_read(BYTE pdrv,  /* Physical drive nmuber to identify the drive */
                  BYTE *buff, /* Data buffer to store read data */
                  DWORD sector, /* Sector address in LBA */
                  UINT count    /* Number of sectors to read */
) {
    DRESULT status = RES_PARERR;
    int result;
    sector += SECTOR_OFFSET;
    switch (pdrv) {
    case INNER_FLASH:
        if (innerflash_fd >= 0) {
            // TODO:@liuzihao TBD
            // flash_read(innerflash_fd, START_ADDRESS + (sector * SECTOR_SIZE),
            //            buff, count * SECTOR_SIZE);
            status = RES_OK;
        }
        break;
    }
    return status;
}

/**
 * @brief  Writes Sector(s)
 * @param  pdrv: Physical drive number (0..)
 * @param  *buff: Data to be written
 * @param  sector: Sector address (LBA)
 * @param  count: Number of sectors to write (1..128)
 * @retval DRESULT: Operation result
 */
DRESULT disk_write(BYTE pdrv, /* Physical drive nmuber to identify the drive */
                   const BYTE *buff, /* Data to be written */
                   DWORD sector,     /* Sector address in LBA */
                   UINT count        /* Number of sectors to write */
) {
    DRESULT status = RES_ERROR;
    sector += SECTOR_OFFSET;
    switch (pdrv) {
    case INNER_FLASH:
        if (innerflash_fd >= 0) {
            // TODO:@liuzihao TBD
            // flash_erasepage(innerflash_fd, sector);
            // flash_write(innerflash_fd, START_ADDRESS + (sector * SECTOR_SIZE),
            //             (const void *)buff, count * SECTOR_SIZE);
            status = RES_OK;
        }
        break;
    }
    return status;
}

/**
 * @brief  I/O control operation
 * @param  pdrv: Physical drive number (0..)
 * @param  cmd: Control code
 * @param  *buff: Buffer to send/receive control data
 * @retval DRESULT: Operation result
 */
DRESULT disk_ioctl(BYTE pdrv, /* Physical drive nmuber (0..) */
                   BYTE cmd,  /* Control code */
                   void *buff /* Buffer to send/receive control data */
) {
    DRESULT res = RES_PARERR;
    int result;
    switch (pdrv) {
        case 0:
            switch (cmd) {
                case CTRL_SYNC:
                    res = RES_OK;
                    break;
                case GET_SECTOR_SIZE:
                    *(DWORD *)buff = SECTOR_SIZE;
                    res = RES_OK;
                    break;
                case GET_BLOCK_SIZE:
                    *(WORD *)buff = BLOCK_SIZE;
                    res = RES_OK;
                    break;
                case GET_SECTOR_COUNT:
                    *(DWORD *)buff = SECTOR_COUNT;
                    res = RES_OK;
                    break;
                default:
                    res = RES_PARERR;
                    break;
            }
            return res;
    }

    return RES_PARERR;
}

/**
 * @brief  Gets Time from RTC
 * @param  None
 * @retval Time in DWORD
 */
__weak DWORD get_fattime(void) { return 0; }

