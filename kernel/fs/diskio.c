#include "fs.h"
#include "fat/diskio.h"
#include "fat/ff.h"
#include <kernel/timer.h>
#include <kernel/drv/disk.h>

/*TODO: Lab7, low level file operator.
 *  You have to provide some device control interface for 
 *  FAT File System Module to communicate with the disk.
 *
 *  Use the function under kernel/drv/disk.c to finish
 *  this part. You can also add some functions there if you
 *  need.
 *
 *  FAT File System Module reference document is under the
 *  doc directory (doc/00index_e.html)
 *
 *  Note:
 *  Since we only use primary slave disk as our file system,
 *  please ignore the pdrv parameter in below function and
 *  just manipulate the hdb disk.
 *
 *  Call flow example:
 *        ┌──────────────┐
 *        │     open     │
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │   sys_open   │  file I/O system call interface
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │  file_open   │  VFS level file API
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │   fat_open   │  fat level file operator
 *        └──────────────┘
 *               ↓
 *        ┌──────────────┐
 *        │    f_open    │  FAT File System Module
 *        └──────────────┘
 *               ↓
 *        ╔══════════════╗
 *   ==>  ║    diskio    ║  low level file operator
 *        ╚══════════════╝
 *               ↓
 *        ┌──────────────┐
 *        │     disk     │  simple ATA disk dirver
 *        └──────────────┘
 */

#define DISK_ID 1

/**
  * @brief  Initial IDE disk
  * @param  pdrv: Physical drive number
  * @retval disk error status
  *         - 0: Initial success
  *         - STA_NOINIT: Intial failed
  *         - STA_PROTECT: Medium is write protected
  */
DSTATUS disk_initialize (BYTE pdrv)
{
  /* TODO */
  /* Note: You can create a function under disk.c  
   *       to help you get the disk status.
   */
  (void)pdrv;
  disk_init();
  return 0;
}

/**
  * @brief  Get disk current status
  * @param  pdrv: Physical drive number
  * @retval disk status
  *         - 0: Normal status
  *         - STA_NOINIT: Device is not initialized and not ready to work
  *         - STA_PROTECT: Medium is write protected
  */
DSTATUS disk_status (BYTE pdrv)
{
/* TODO */
/* Note: You can create a function under disk.c  
 *       to help you get the disk status.
 */
  unsigned char status = ide_read(ATA_PRIMARY, ATA_REG_STATUS);
  if (status == ATA_SR_DRDY)
    return 0;
  return STA_NOINIT;
}

/**
  * @brief  Read serval sector form a IDE disk
  * @param  pdrv: Physical drive number
  * @param  buff: destination memory start address
  * @param  sector: start sector number
  * @param  count: number of sector
  * @retval Results of Disk Functions (See diskio.h)
  *         - RES_OK: success
  *         - < 0: failed
  */
DRESULT disk_read (BYTE pdrv, BYTE* buff, DWORD sector, UINT count)
{
    (void)pdrv;
    int err = ide_read_sectors(DISK_ID, count, sector, (unsigned int)buff);
    if (err == 0)
        return RES_OK;
    return -RES_ERROR;
}

/**
  * @brief  Write serval sector to a IDE disk
  * @param  pdrv: Physical drive number
  * @param  buff: memory start address
  * @param  sector: destination start sector number
  * @param  count: number of sector
  * @retval Results of Disk Functions (See diskio.h)
  *         - RES_OK: success
  *         - < 0: failed
  */
DRESULT disk_write (BYTE pdrv, const BYTE* buff, DWORD sector, UINT count)
{
    (void)pdrv;
    int err = ide_write_sectors(DISK_ID, count, sector, (unsigned int)buff);
    if (err == 0)
        return RES_OK;
    return -RES_ERROR;
}

/**
  * @brief  Get disk information form disk
  * @param  pdrv: Physical drive number
  * @param  cmd: disk control command (See diskio.h)
  *         - GET_SECTOR_COUNT
  *         - GET_BLOCK_SIZE (Same as sector size)
  * @param  buff: return memory space
  * @retval Results of Disk Functions (See diskio.h)
  *         - RES_OK: success
  *         - < 0: failed
  */
DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void* buff)
{
    /* TODO */    
    (void)pdrv;
    uint32_t *retVal = (uint32_t *)buff;
    DRESULT ret = -RES_PARERR;
    switch (cmd) {
    case CTRL_SYNC:
        ret = RES_OK;
        break;
    case GET_SECTOR_COUNT:
        *retVal = ide_devices[DISK_ID].Size;
        ret = RES_OK;
        break;
    case GET_SECTOR_SIZE:
        *retVal = SECTOR_SIZE;
        ret = RES_OK;
        break;
    case GET_BLOCK_SIZE:
        *retVal = SECTOR_SIZE;
        ret = RES_OK;
        break;
    case CTRL_TRIM:
        ret = RES_OK;
        break;
    }
    return ret;
}

/**
  * @brief  Get OS timestamp
  * @retval tick of CPU
  */
DWORD get_fattime (void)
{
    /* TODO */
    return get_tick();
}
