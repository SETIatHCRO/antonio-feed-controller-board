/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for Petit FatFs (C)ChaN, 2009      */
/*-----------------------------------------------------------------------*/

#include <plib.h>

#include "diskio.h"

#undef MYDISKIODEBUG

#ifdef MYDISKIODEBUG
void mylog(char *msg);
#endif

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (void)
{
    DSTATUS stat;
#ifdef MYDISKIODEBUG
    char msg[99];
#endif
    unsigned char spi_c;

#ifdef MYDISKIODEBUG
    mylog("disk_initialize()");
#endif

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
        SpiChnPutC(1, 0x00);  // clear BP3, BP2, BP1, and BP0
        spi_c = SpiChnGetC(1);
        mPORTDSetBits(BIT_9);
        // read status
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
    }

    stat = RES_OK;

    return stat;
}

/*-----------------------------------------------------------------------*/
/* Read Partial Sector                                                   */
/*-----------------------------------------------------------------------*/

DRESULT disk_readp (
    BYTE* dest,			/* Pointer to the destination object */
    DWORD sector,		/* Sector number (LBA) */
    WORD sofs,			/* Offset in the sector */
    WORD count			/* Byte count (bit15:destination) */
)
{
    DRESULT res;

#ifdef MYDISKIODEBUG
    char msg[99];
#endif
    unsigned int i;
    unsigned int address;
    unsigned char spi_c;

    address = (sector * 512) + sofs;

#ifdef MYDISKIODEBUG
    sprintf(msg, "disk_readp(dest, %u, %u, %u)", sector, sofs, count);
    mylog(msg);
#endif

    // read data
    mPORTDClearBits(BIT_9);
    SpiChnPutC(1, 0x03);
    spi_c = SpiChnGetC(1);
    SpiChnPutC(1, (unsigned char) ((address >> 16) & 0x0ff));
    spi_c = SpiChnGetC(1);
    SpiChnPutC(1, (unsigned char) ((address >> 8) & 0x0ff));
    spi_c = SpiChnGetC(1);
    SpiChnPutC(1, (unsigned char) ((address >> 0) & 0x0ff));
    spi_c = SpiChnGetC(1);
    for (i = 0; i < count; i++) {
        SpiChnPutC(1, 0x00);
        spi_c = SpiChnGetC(1);
        dest[i] = spi_c;
    }
    mPORTDSetBits(BIT_9);

    res = RES_OK;

    return res;
}

/*-----------------------------------------------------------------------*/
/* Write Partial Sector                                                  */
/*-----------------------------------------------------------------------*/

DRESULT disk_writep (const 
    BYTE* buff,		/* Pointer to the data to be written,      */
                        /*  NULL:Initiate/Finalize write operation */
    DWORD sc		/* Sector number (LBA) or Number of bytes to send */
)
{
    DRESULT res;

    if (!buff) {
        if (sc) {
            // Initiate write process
        } else {
            // Finalize write process
        }
    } else {
        // Send data to the disk
    }

    return res;
}

/*
 * Erase and Write 4K sector
 */
DRESULT disk_writes (unsigned int sector, unsigned char *bfr) {
    unsigned int i;
#ifdef MYDISKIODEBUG
    char msg[99];
#endif
    unsigned char spi_c;

    unsigned int sector_addr;
    unsigned int page_addr;

    unsigned int page_num;
    unsigned int bfri;

    DRESULT res;

#ifdef MYDISKIODEBUG
    sprintf(msg, "disk_writes(%u, bfr)", sector);
    mylog(msg);
#endif

    sector_addr = (sector << 12) & 0x0fff000;

    // write enable
    mPORTDClearBits(BIT_9);
    SpiChnPutC(1, 0x06);
    spi_c = SpiChnGetC(1);
    mPORTDSetBits(BIT_9);
    // 4K sector erase
    mPORTDClearBits(BIT_9);
    SpiChnPutC(1, 0x20);
    spi_c = SpiChnGetC(1);
    SpiChnPutC(1, (unsigned char) ((sector_addr >> 16) & 0x0ff));
    spi_c = SpiChnGetC(1);
    SpiChnPutC(1, (unsigned char) ((sector_addr >> 8) & 0x0ff));
    spi_c = SpiChnGetC(1);
    SpiChnPutC(1, (unsigned char) ((sector_addr >> 0) & 0x0ff));
    spi_c = SpiChnGetC(1);
    mPORTDSetBits(BIT_9);
    // read status waiting until erase complete
    mPORTDClearBits(BIT_9);
    SpiChnPutC(1, 0x05);
    spi_c = SpiChnGetC(1);
    do {
        SpiChnPutC(1, 0x00);
        spi_c = SpiChnGetC(1);
    }
    while (spi_c & 0x01);  // while busy (erase operation in progress)
    mPORTDSetBits(BIT_9);

    // write 16 256-byte pages to 4K sector
    bfri = 0;
    for (page_num = 0; page_num < 16; page_num++) {
        // write enable
        mPORTDClearBits(BIT_9);
        SpiChnPutC(1, 0x06);
        spi_c = SpiChnGetC(1);
        mPORTDSetBits(BIT_9);
        page_addr = sector_addr | ((page_num << 8) & 0x0f00);
        // page program 256 bytes of memory
        mPORTDClearBits(BIT_9);
        SpiChnPutC(1, 0x02);
        spi_c = SpiChnGetC(1);
        SpiChnPutC(1, (unsigned char) ((page_addr >> 16) & 0x0ff));
        spi_c = SpiChnGetC(1);
        SpiChnPutC(1, (unsigned char) ((page_addr >> 8) & 0x0ff));
        spi_c = SpiChnGetC(1);
        SpiChnPutC(1, (unsigned char) ((page_addr >> 0) & 0x0ff));
        spi_c = SpiChnGetC(1);
        // write data to memory from buffer
        for (i = 0; i < 256; i++) {
            SpiChnPutC(1, bfr[bfri++]);
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
        }
        while (spi_c & 0x01);  // while busy (write operation in progress)
        mPORTDSetBits(BIT_9);
    }
    
    res = RES_OK;

    return res;
}