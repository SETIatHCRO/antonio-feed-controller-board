/* 
 * File:   main.c
 * Author: Jon's Windows
 *
 * Created on January 7, 2019, 8:01 AM
 */

#ifdef __XC32
#endif

#include <plib.h>           /// Include to use PIC32 peripheral libraries

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>        /* For true/false definition     */
#include <string.h>

#include "system.h" 
#include "user.h"           // UserInitApp(), delayMs()
#include "fatfs/diskio.h"
#include "fatfs/ff.h"
#include "oneshot.h"
#include "diode.h"          // JR - added this Jan 7, 2019, to remove from main
#include "rimbox.h"
#include "cryo.h"
#include "vacuum.h"
#include "autostart.h"
#include "fan.h"
#include "commands.h"

#define MYDEBUG

float version = 4.2;
static char init_string[] = "Antonio Feed Monitor and Control Firmware";

bool echo_mode = false;
bool is_rimbox_session_inactive = false;

char CR[]   = "\r";
char CRLF[] = "\r\n";
char VBAR[] = "|";

char OK[] = "OK";
char ERROR[] = "error";

char TIMEOUT[] = "timeout";

char *LINESEP = VBAR;
char *EOL = CR;


void (*poll_auto_start)();
void init_log();
void toggle_watchdog();
void stty_command(char *args[]);

/*
 * Z:\seti\antonio_v2\antonio_feed_control_v2.X\accel_1.c
 */
int main(void) {
    
    FRESULT res;
    FATFS fso;
    char msg[99];

    uint32_t start_time;
    uint32_t end_time;

    SYSTEMConfig(SYS_FREQ, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);
    
    OpenCoreTimer(CORE_TIMER_PERIOD);
    mConfigIntCoreTimer((CT_INT_ON | CT_INT_PRIOR_2 | CT_INT_SUB_PRIOR_0));
    
    INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);

    INTEnableSystemMultiVectoredInt();  // includes INTConfigureSystem() ?
    
    // delay for three seconds before getting started.  This prevents
    // annoying situation caused when microcontroller is reset multiple
    // times during programming.
    delayMs(3000);
    
    // Init to stty mode. This is necessary because these values don't get
    // re-initialized if a soft reset command is issued.
    stty_command(NULL);
     
    /* Initialize I/O and Peripherals for application */
    InitApp();
    
    delay_and_turn_on_i2c();

    init_oneshot();
    
    poll_recv_from_rimbox = purge_chars_from_rimbox;
    poll_cryo_session = cryo_poll_idle;
    poll_vac_session = vac_poll_idle;
    poll_auto_start = auto_start_idle;

    res = f_mount(&fso, "", 1);  // mount the external flash filesystem
    
    init_log();

    init_tc74();
    
    init_adc();

    init_fan();
    
    cryo_init();
    
    EnableWDT();
    
    sprintf(msg, "%s %4.2f", init_string, version);
    send_to_rimbox(msg);
    send_to_rimbox(EOL);
    feedlog(msg);
   

    //load_init();

    // Relay controls the cryo on/off
    // Do not init the state of the relay, this is done
    // in cryo_init())
    //load_relay_state();

    load_fan_state();

    load_autostart_state();

    sprintf(msg, "feed start mode is %s",
        (get_auto_start_state() == true ? "automatic": "manual"));
    feedlog(msg);
    
    init_accel();
    
    while(1) {
        toggle_watchdog();
    
//mylog("0");
        poll_recv_from_rimbox();
        poll_accel();
        poll_oneshot_timers();

        poll_send_to_rimbox();
        poll_accel();
        poll_oneshot_timers();

        poll_vac_session();
        poll_accel();
        poll_oneshot_timers();

        poll_cryo_session();
        poll_accel();
        poll_oneshot_timers();

        poll_vac_session();
        poll_accel();
        poll_oneshot_timers();

        poll_cryo_session();
        poll_accel();
        poll_oneshot_timers();        

        poll_accel();
        poll_oneshot_timers();

        poll_adc();
        poll_accel();
        poll_oneshot_timers();
//mylog("1");
        poll_tc74();
        poll_accel();
        poll_oneshot_timers();
//mylog("2");
        poll_fan_sense();
        poll_accel();
        poll_oneshot_timers();
//mylog("3");

        poll_auto_start();
//mylog("4");

    }
    
    return (EXIT_SUCCESS);
}

void toggle_watchdog() {
        if (is_rimbox_session_inactive) {
            return;
        }
    
        ClearWDT();
}

void bootloader_command(char *args[]) {
    SoftReset();
}

void getversion_command(char *args[]) {
    char msg[19];

    sprintf(msg, "%4.2f", version);

    send_to_rimbox(msg);
    send_to_rimbox(EOL);
}

void compute_checksum_if_requested(char *command) {
    unsigned int i;
    char checksum_digits[9];
    unsigned int N = (unsigned int)strlen(command);
    uint8_t checksum = 0;

    if (!(N > 3)) {
        return;
    }

    if ((command[N-3] == '#') && (command[N-2] == '#') &&
            (command[N-1] == '#')) {
        for (i = 0; i < (N - 3); i++) {
            checksum = checksum + command[i];
        }
        sprintf(checksum_digits, "%03u", checksum);
        command[N-3] = checksum_digits[0];
        command[N-2] = checksum_digits[1];
        command[N-1] = checksum_digits[2];
    }
}

/*
 * open LOG.TXT in the flash filesystem and truncate it to zero length
 */
void init_log() {
    FIL fp;
    FRESULT rslt;

    rslt = f_open(&fp, "LOG.TXT", (FA_CREATE_ALWAYS | FA_WRITE));
    if (rslt == FR_OK) {
        f_close(&fp);
    }
}



#ifdef MYDEBUG
void mylog (char *msg) {
    send_to_rimbox(msg);
    //send_to_rimbox(EOL);
}
#endif