/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2014        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include <plib.h>

#include "diskio.h"		/* FatFs lower layer API */

#undef MYDISKIODEBUG

#ifdef MYDISKIODEBUG
void mylog(char *msg);
#endif

UINT is_disk_initialized = 0;

#define NBLKS 3

unsigned char is_cached[NBLKS]; // if sectors have been read into block from flash
unsigned char is_dirty[NBLKS]; // if sectors have been written into block, but not to flash

DWORD boundary_sector[NBLKS]; // number of first sector of block

#define SCTRS_PER_BLK 8

BYTE bfr4K[NBLKS][SCTRS_PER_BLK][512]; // 8 * 512 bytes sectors = 4096 bytes

unsigned char is_in_cache(DWORD sector);
unsigned int get_block_idx(DWORD sector);
BYTE* get_sector_ptr(DWORD sector);
void refresh_cache(DWORD sector);
void read_from_cache(BYTE* buff, DWORD sector);
DRESULT read_sector_from_flash(BYTE* buff, DWORD sector);
void write_to_cache(DWORD sector, const BYTE* buff);
DRESULT read_block_from_flash(BYTE* block_ptr, DWORD sector);
DRESULT write_block_to_flash(BYTE* block_ptr, DWORD sector);


/*
 * Description of sector cache
 *
 * sectors are cached in 4096 byte blocks, which is the smallest
 * flash erase and write block size
 *
 * disk_read
 *   for each sector:
 *     if in cache, read sector from cache, else read from flash
 *
 * disk_write
 *   for each sector:
 *     if sector not in cache
 *       if all cache blocks dirty
 *         write cache block with highest boundary sector value to flash and
 *             then mark not dirty
 *       if free cache block
 *         read 4K block containing sector into free cache block
 *       else
 *         read 4K block containing sector into cache block that has highest
 *             boundary sector value and is not dirty
 *     write sector to cache and mark cache block dirty
 *
 * disk_ioctl (CTRL_SYNC)
 *   flush dirty cache blocks to flash by doing erase and write and then
 *       mark not dirty
 */

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */

/*-----------------------------------------------------------------------*/

DSTATUS disk_status(
        BYTE pdrv /* Physical drive nmuber to identify the drive */
        ) {
    DSTATUS stat;
    int result;

#ifdef MYDISKIODEBUG
    char msg[99];
#endif

#ifdef MYDISKIODEBUG
    //        mylog("disk_status()");
#endif

    if (is_disk_initialized) {
        return 0;
    }

    return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */

/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(
        BYTE pdr /* Physical drive nmuber to identify the drive */
        ) {
    DSTATUS stat;
    DRESULT result;

#ifdef MYDISKIODEBUG
    char msg[99];
#endif
    unsigned int i;
    unsigned char spi_c;

#ifdef MYDISKIODEBUG
    mylog("disk_initialize()");
#endif

    // just in case disk_initialize called more than once
    if (is_disk_initialized) {
        return (0);
    }

    for (i = 0; i < NBLKS; i++) {
        is_cached[i] = 0;
        is_dirty[i] = 0;
    }

    // read status register
    mPORTDClearBits(BIT_9);
    SpiChnPutC(1, 0x05);
    spi_c = SpiChnGetC(1);
    SpiChnPutC(1, 0x00);
    spi_c = SpiChnGetC(1);
    mPORTDSetBits(BIT_9);

#ifdef MYDISKIODEBUG
    sprintf(msg, "status register %02x", spi_c);
    mylog(msg);
#endif

    // if block write protection, turn off
    if (spi_c & 0x3c) {
        // write enable
        mPORTDClearBits(BIT_9);
        SpiChnPutC(1, 0x06);
        spi_c = SpiChnGetC(1);
        mPORTDSetBits(BIT_9);
        // write status register
        mPORTDClearBits(BIT_9);
        SpiChnPutC(1, 0x01);
        spi_c = SpiChnGetC(1);
        SpiChnPutC(1, 0x00); // clear BP3, BP2, BP1, and BP0
        spi_c = SpiChnGetC(1);
        mPORTDSetBits(BIT_9);
    }

    // read status again
    mPORTDClearBits(BIT_9);
    SpiChnPutC(1, 0x05);
    spi_c = SpiChnGetC(1);
    SpiChnPutC(1, 0x00);
    spi_c = SpiChnGetC(1);
    mPORTDSetBits(BIT_9);

#ifdef MYDISKIODEBUG
    sprintf(msg, "status register %02x", spi_c);
    mylog(msg);
#endif

    stat = STA_NOINIT;

    if (!(spi_c & 0x3c)) {
        is_disk_initialized = 1;
        stat = 0;
        result = read_block_from_flash(&bfr4K[0][0][0], 0);
        if (result == RES_OK ) {
            is_cached[0] = 1;
            boundary_sector[0] = 0;
        }
    }

    // next seven lines are for test.  They should be commented
    // out in production.
//    read_block_from_flash(&bfr4K[1][0][0], 520);
//    is_cached[1] = 1;
//    is_dirty[1] = 1;
//    boundary_sector[1] = 520;
//    read_block_from_flash(&bfr4K[2][0][0], 16);
//    is_cached[2] = 1;
//    boundary_sector[2] = 16;

    return stat;
}


/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */

/*-----------------------------------------------------------------------*/

DRESULT disk_read(
        BYTE pdrv, /* Physical drive nmuber to identify the drive */
        BYTE *buff, /* Data buffer to store read data */
        DWORD sector, /* Sector address in LBA */
        UINT count /* Number of sectors to read */
        ) {
    DRESULT res;
    int result;

#ifdef MYDISKIODEBUG
    char msg[99];
#endif

    unsigned int i;
 
#ifdef MYDISKIODEBUG
    sprintf(msg, "disk_read(pdrv, buff, %u, %u)", sector, count);
    mylog(msg);
#endif

    res = RES_OK;

    for (i = 0; i < count; i++) {
        if (is_in_cache(sector)) {
            read_from_cache(buff, sector);
        }
        else {
            res = read_sector_from_flash(buff, sector);
        }
        sector += 1;
        buff += 512;
    }

    return res;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE

DRESULT disk_write(
        BYTE pdrv, /* Physical drive nmuber to identify the drive */
        const BYTE *buff, /* Data to be written */
        DWORD sector, /* Sector address in LBA */
        UINT count /* Number of sectors to write */
        ) {
    DRESULT res;
    int result;

#ifdef MYDISKIODEBUG
    char msg[99];
#endif

    unsigned int i;

#ifdef MYDISKIODEBUG
    sprintf(msg, "disk_write(pdrv, buff, %u, %u)", sector, count);
    mylog(msg);
#endif

    res = RES_OK;

    for (i = 0; i < count; i++) {
        if (!(is_in_cache(sector))) {
            refresh_cache(sector);
        }
        write_to_cache(sector, buff + (i * 512));
        is_dirty[get_block_idx(sector)] = 1;
        sector += 1;
    }

    return res;
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

#if _USE_IOCTL

DRESULT disk_ioctl(
        BYTE pdrv, /* Physical drive nmuber (0..) */
        BYTE cmd, /* Control code */
        void *buff /* Buffer to send/receive control data */
        ) {
    DRESULT res;
    int result;

    unsigned int i;

    DWORD sector;

#ifdef MYDISKIODEBUG
    char msg[99];
#endif

#ifdef MYDISKIODEBUG
    sprintf(msg, "disk_ioctl(pdrv, %u, buff)", (UINT) cmd);
    mylog(msg);
#endif

    if (cmd == CTRL_SYNC) {
        for (i = 0; i < NBLKS; i++) {
            if (is_dirty[i]) {
                sector = boundary_sector[i];
                write_block_to_flash(get_sector_ptr(sector), sector);
                is_dirty[i] = 0;
            }
        }
        return RES_OK;
    }

    return RES_PARERR;
}
#endif

unsigned char is_in_cache(DWORD sector) {
    unsigned int i;

    for (i = 0; i < NBLKS; i++) {
        if (is_cached[i] && (sector >= boundary_sector[i]) &&
                (sector < (boundary_sector[i] + 8))) {
            return (1); // true
        }
    }

    return (0); // false
}

/*
 * assumes is_in_cache previously called and returned true
 */
unsigned int get_block_idx(DWORD sector) {
    unsigned int i;

    for (i = 0; i < NBLKS; i++) {
        if (is_cached[i] && (sector >= boundary_sector[i]) &&
                (sector < (boundary_sector[i] + 8))) {
            return (i);
        }
    }

    while (1) {};  // should not get here
}

/*
 * assumes is_in_cache previously called and returned true
 */
BYTE* get_sector_ptr(DWORD sector) {
    unsigned int i = get_block_idx(sector);

    return (&bfr4K[i][sector - boundary_sector[i]][0]);
}

/*
 * if necessary to make room, write a cache block to flash.  Then read
 * a cache block from flash containing the called sector.
 */
void refresh_cache(DWORD sector) {
    unsigned int i;
    
    DWORD max_boundary_sector = 0;

    BYTE* buff;

    for (i = 0; i < NBLKS; i++) {
        if (!(is_cached[i])) {
            boundary_sector[i] = (sector & 0xfffffff8);
            read_block_from_flash(&bfr4K[i][0][0] , boundary_sector[i]);
            is_cached[i] = 1;
            return;
        }
        if (boundary_sector[i] > max_boundary_sector) {
            max_boundary_sector = boundary_sector[i];
        }
    }

    i = get_block_idx(max_boundary_sector);
    
    if (is_dirty[i]) {
        buff = get_sector_ptr(max_boundary_sector);
        write_block_to_flash(buff, max_boundary_sector);
    }

    boundary_sector[i] = (sector & 0xfffffff8);

    read_block_from_flash(&bfr4K[i][0][0] , boundary_sector[i]);

    is_cached[i] = 1;
    is_dirty[i] = 0;
}

void read_from_cache(BYTE* buff, DWORD sector) {
    unsigned int i;

#ifdef MYDISKIODEBUG
    char msg[99];
#endif

#ifdef MYDISKIODEBUG
    sprintf(msg, "read_from_cache(buff, %u)", sector);
    mylog(msg);
#endif

    BYTE* sector_ptr = get_sector_ptr(sector);

    for (i = 0; i < 512; i++) {
        *buff = *sector_ptr;
        buff++;
        sector_ptr++;
    }
}

DRESULT read_sector_from_flash(BYTE* buff, DWORD sector) {
    unsigned int i;
    DWORD address;
    unsigned char spi_c;

#ifdef MYDISKIODEBUG
    char msg[99];
#endif

#ifdef MYDISKIODEBUG
    sprintf(msg, "read_sector_from_flash(buff, %u)", sector);
    mylog(msg);
#endif

    address = sector * 512;

    mPORTDClearBits(BIT_9);
    SpiChnPutC(1, 0x03);
    spi_c = SpiChnGetC(1);
    SpiChnPutC(1, (unsigned char) ((address >> 16) & 0x0ff));
    spi_c = SpiChnGetC(1);
    SpiChnPutC(1, (unsigned char) ((address >> 8) & 0x0ff));
    spi_c = SpiChnGetC(1);
    SpiChnPutC(1, (unsigned char) ((address >> 0) & 0x0ff));
    spi_c = SpiChnGetC(1);
    for (i = 0; i < 512; i++) {
        SpiChnPutC(1, 0x00);
        spi_c = SpiChnGetC(1);
        *buff = spi_c;
        buff++;
    }
    mPORTDSetBits(BIT_9);

    return (RES_OK);
}

void write_to_cache(DWORD sector, const BYTE* buff) {
    unsigned int i;

#ifdef MYDISKIODEBUG
    char msg[99];
#endif

#ifdef MYDISKIODEBUG
    sprintf(msg, "write_to_cache(%u, buff)", sector);
    mylog(msg);
#endif

    BYTE* sector_ptr = get_sector_ptr(sector);

    for (i = 0; i < 512; i++) {
        *sector_ptr = *buff;
        buff++;
        sector_ptr++;
    }

}

DRESULT read_block_from_flash(BYTE* block_ptr, DWORD sector) {

    DRESULT res;

    unsigned int i;

    BYTE* buff;

#ifdef MYDISKIODEBUG
    char msg[99];
#endif

#ifdef MYDISKIODEBUG
    sprintf(msg, "read_block_from_flash(block_ptr, %u)", sector);
    mylog(msg);
#endif

    buff = block_ptr;

    for (i = 0; i < SCTRS_PER_BLK; i++) {
        read_sector_from_flash(buff, sector);
        sector += 1;
        buff += 512;
    }

    return (RES_OK);
}

DRESULT write_block_to_flash(BYTE* block_ptr, DWORD sector) {

    unsigned char spi_c;

    unsigned int i;
    unsigned int pagenum;

    DWORD address;
    DWORD pageaddr;

#ifdef MYDISKIODEBUG
    char msg[99];
#endif

#ifdef MYDISKIODEBUG
    sprintf(msg, "write_block_to_flash(block_ptr, %u)", sector);
    mylog(msg);
#endif

    address = sector * 512;

    // write enable
    mPORTDClearBits(BIT_9);
    SpiChnPutC(1, 0x06);
    spi_c = SpiChnGetC(1);
    mPORTDSetBits(BIT_9);
    // 4K sector erase
    mPORTDClearBits(BIT_9);
    SpiChnPutC(1, 0x20);
    spi_c = SpiChnGetC(1);
    SpiChnPutC(1, (unsigned char) ((address >> 16) & 0x0ff));
    spi_c = SpiChnGetC(1);
    SpiChnPutC(1, (unsigned char) ((address >> 8) & 0x0ff));
    spi_c = SpiChnGetC(1);
    SpiChnPutC(1, (unsigned char) ((address >> 0) & 0x0ff));
    spi_c = SpiChnGetC(1);
    mPORTDSetBits(BIT_9);
    // read status waiting until erase complete
    mPORTDClearBits(BIT_9);
    SpiChnPutC(1, 0x05);
    spi_c = SpiChnGetC(1);
    do {
        SpiChnPutC(1, 0x00);
        spi_c = SpiChnGetC(1);
    } while (spi_c & 0x01); // while busy (erase operation in progress)
    mPORTDSetBits(BIT_9);

    // write 16 256-byte pages to 4K sector
    for (pagenum = 0; pagenum < 16; pagenum++) {
        // write enable
        mPORTDClearBits(BIT_9);
        SpiChnPutC(1, 0x06);
        spi_c = SpiChnGetC(1);
        mPORTDSetBits(BIT_9);
        pageaddr = address + (pagenum << 8);
        // page program 256 bytes of memory
        mPORTDClearBits(BIT_9);
        SpiChnPutC(1, 0x02);
        spi_c = SpiChnGetC(1);
        SpiChnPutC(1, (unsigned char) ((pageaddr >> 16) & 0x0ff));
        spi_c = SpiChnGetC(1);
        SpiChnPutC(1, (unsigned char) ((pageaddr >> 8) & 0x0ff));
        spi_c = SpiChnGetC(1);
        SpiChnPutC(1, (unsigned char) ((pageaddr >> 0) & 0x0ff));
        spi_c = SpiChnGetC(1);
        // write data to memory from buffer
        for (i = 0; i < 256; i++) {
            SpiChnPutC(1, *block_ptr);
            block_ptr++;
            spi_c = SpiChnGetC(1);
        }
        mPORTDSetBits(BIT_9);
        // read status waiting until write complete
        mPORTDClearBits(BIT_9);
        SpiChnPutC(1, 0x05);
        spi_c = SpiChnGetC(1);
        do {
            SpiChnPutC(1, 0x00);
            spi_c = SpiChnGetC(1);
        } while (spi_c & 0x01); // while busy (write operation in progress)
        mPORTDSetBits(BIT_9);
    }

    return (RES_OK);
}
