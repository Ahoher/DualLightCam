#include "diskio.h"
#include "ff_gen_drv.h"

extern Diskio_drvTypeDef USER_Driver;

DSTATUS disk_initialize(BYTE pdrv) {
    return USER_Driver.disk_initialize(pdrv);
}

DSTATUS disk_status(BYTE pdrv) {
    return USER_Driver.disk_status(pdrv);
}

DRESULT disk_read(BYTE pdrv, BYTE* buff, DWORD sector, UINT count) {
    return USER_Driver.disk_read(pdrv, buff, sector, count);
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, DWORD sector, UINT count) {
    return USER_Driver.disk_write(pdrv, buff, sector, count);
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff) {
    return USER_Driver.disk_ioctl(pdrv, cmd, buff);
}
