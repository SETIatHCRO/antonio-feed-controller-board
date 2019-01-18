#ifdef __XC32
    #include <xc.h>          /* Defines special funciton registers, CP0 regs  */
#endif

#include <plib.h>           /* Include to use PIC32 peripheral libraries      */

#include <string.h>
#include <stdio.h>
#include <stdbool.h>        /* For true/false definition     */

#include "relay.h"
#include "rimbox.h"
#include "fatfs/ff.h"
#include "file_utils.h"

extern char *EOL;
extern char OK[];

bool relay_state = false;

void setrelay_command(char *args[]) {

    if (args[0] == NULL) {
        send_to_rimbox(EOL);
        return;
    }

    if (strcasecmp(args[0], "off") == 0) {
        mPORTGClearBits(BIT_0);
        relay_state = false;
        save_relay_state();
        send_to_rimbox(OK);
        send_to_rimbox(EOL);
        return;
    }

    if (strcasecmp(args[0], "on") == 0) {
        mPORTGSetBits(BIT_0);
        relay_state = true;
        save_relay_state();
        send_to_rimbox(OK);
        send_to_rimbox(EOL);
        return;
    }
 
    // command parameter not "off" or "on"
    send_to_rimbox(EOL);
    return;
}

void getrelay_command(char *args[]) {
    char msg[19];

    if (relay_state == true) {
        strcpy(msg, "on");
    }
    else {
        strcpy(msg, "off");
    }

    send_to_rimbox(msg);
    send_to_rimbox(EOL);
}

void save_relay_state() {
    FIL fp;
    FRESULT rslt;
    UINT bytes_written;

    char text_state[9];

    strcpy(text_state, (relay_state == true ? "on" : "off"));

    rslt = f_open(&fp, "RLYSTATE.TXT", (FA_CREATE_ALWAYS | FA_WRITE));
    if (rslt == FR_OK) {
        f_write(&fp, text_state, strlen(text_state), &bytes_written);
        f_close(&fp);
    }
}

void load_relay_state() {
    FIL fp;
    FRESULT rslt;
    UINT bytes_read;

    char text_state[19];

    relay_state = false;

    rslt = f_open(&fp, "RLYSTATE.TXT", (FA_READ));
    if (rslt == FR_OK) {
        f_read(&fp, text_state, 3, &bytes_read);
        f_close(&fp);
        if (bytes_read >= 2) {
            text_state[bytes_read] = 0;
            if (strcasecmp(text_state, "on") == 0) {
                mPORTGSetBits(BIT_0);
                relay_state = true;
            }
        }
    }
}