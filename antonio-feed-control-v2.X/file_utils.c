#include <plib.h>           /* Include to use PIC32 peripheral libraries      */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>        /* For true/false definition     */

#include "system.h"

#include "file_utils.h"
#include "rimbox.h"
#include "fatfs/diskio.h"
#include "fatfs/ff.h"

extern bool echo_mode;
extern char *EOL;
extern char *LINESEP;
extern char CRLF[];
extern char OK[];
extern char VBAR[];
extern char CR[];

bool update_logs = true;

/*
 * list files in directory of flash filesystem
 */
void ls_command(char *args[]) {
    char msg[99];

    FRESULT res;
    FILINFO fno;
    DIR dir;
    int i;

    res = f_opendir(&dir, (args[0] == NULL ? "" : args[0]));
    if (res == FR_OK) {
        for (i = 0;; i++) {
            res = f_readdir(&dir, &fno);
            if ((res != FR_OK) || (fno.fname[0] == 0)) {
                break;
            }
            if (i > 0) {
                send_to_rimbox(LINESEP);
            }
            if (fno.fattrib & AM_DIR) {
                sprintf(msg, "%-12s  %7s", fno.fname, "<DIR>");
            }
            else {
                sprintf(msg, "%-12s  %7u", fno.fname, fno.fsize);
            }
            send_to_rimbox(msg);
        }
    }

    send_to_rimbox(EOL);
}


/*
 * display contents of .DAT and .TXT files stored in flash filesystem.
 * Files with other extensions are excluded because they might be
 * large and/or binary.
 */
void cat_command(char *args[]) {
    unsigned int i;
    char cat_char;

#define CAT_READ_BFR_LEN 80
    char read_bfr[CAT_READ_BFR_LEN];

    FIL fp;

    FRESULT res;
    UINT read_count;

    char *pext;

    if (args[0] == NULL) {
        send_to_rimbox(EOL);
        return;
    }

    pext = strrchr(args[0], '.');

    if (pext == NULL) {
        send_to_rimbox(EOL);
        return;
    }

    if ((strcasecmp(pext, ".dat") != 0) && (strcasecmp(pext, ".txt") != 0)) {
        send_to_rimbox(EOL);
        return;
    }

    res = f_open(&fp, args[0], FA_READ);
    if (res == FR_OK) {
        do {
            res = f_read(&fp, read_bfr, (UINT) CAT_READ_BFR_LEN, &read_count);
            if (!(res == FR_OK)) {
                break;
            }
            if (read_count > 0) {
                for (i = 0; i < read_count; i++) {
                    cat_char = read_bfr[i];
                    if (cat_char == 0x0a) {
                        send_to_rimbox(LINESEP);
                    }
                    if ((cat_char >= 0x20) && (cat_char <= 0x7e)) {
                        send_char_to_rimbox(cat_char);
                    }
                }
            }
        }
        while (read_count == CAT_READ_BFR_LEN);
    }

    send_to_rimbox(EOL);
}

/*
 * this puts the control board in a mode best for interaction with
 * a terminal (such as at Minex)
 */
void hyperterminal_command(char *args[]) {
    echo_mode = true;

    EOL = CRLF;
    LINESEP = EOL;

    send_to_rimbox(OK);
    send_to_rimbox(EOL);
}

/*
 * this puts the control board in a mode suitable for interaction
 * through the rimbox controller virtual TCP/IP to RS-232 port.  Most
 * important is the use of '|' as a line separator.  The virtual link
 * only supports return of a single line so multiple line responses
 * are separated by the '|' character.
 */
void stty_command(char *args[]) {
    echo_mode = false;

    EOL = CR;
    LINESEP = VBAR;

    send_to_rimbox(OK);
    send_to_rimbox(EOL);
}

/*
 * append a message to LOG.TXT in the flash filesystem
 */
void feedlog(char * msg)
{
    if(update_logs)
    {
        feedlog_always(msg);
    }
}

void feedlog_always(char *msg) {
    FIL fp;
    FRESULT rslt;
    UINT bytes_written;

    char msgwithlf[99];

    strcpy (msgwithlf, msg);
    strcat (msgwithlf, "\n");

    rslt = f_open(&fp, "LOG.TXT", (FA_OPEN_ALWAYS | FA_WRITE));
    if (rslt == FR_OK) {
        rslt = f_lseek(&fp, f_size(&fp));
        if (rslt == FR_OK) {
            f_write(&fp, msgwithlf, strlen(msgwithlf), &bytes_written);
        }
        f_close(&fp);
    }
}
