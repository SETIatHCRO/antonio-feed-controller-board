
#ifdef __XC32
    #include <xc.h>          /* Defines special funciton registers, CP0 regs  */
#endif

#include <plib.h>           /* Include to use PIC32 peripheral libraries      */

#include <stdbool.h>        /* For true/false definition     */
#include <string.h>

#include "cryo.h"
#include "rimbox.h"
#include "oneshot.h"
#include "relay.h"
#include "fatfs/ff.h"
#include "file_utils.h"

extern char TIMEOUT[];
extern char *LINESEP;
extern char *EOL;
extern char OK[];

extern bool relay_state;

struct oneshot_timer cryo_rspns_timeout_timer = {"cryo_response_timeout", 0, NULL, NULL};
struct oneshot_timer cryo_rspns_eol_timer = {"cryo_eol_timeout", 0, NULL, NULL};

#define MAX_COMMAND_LEN 999
#define MAX_RESPONSE_LEN 999

unsigned int cryo_req_i = 0;
char cryo_request[MAX_COMMAND_LEN];
unsigned int cryo_rspns_i = 0;
char cryo_response[MAX_RESPONSE_LEN];
bool is_cryo_busy = false;
bool is_cryo_response_ready = false;

#define MAX_ARGS 99
char *args[MAX_ARGS];
char *command;

/*
bool cryostopmode = false;
void send_cryostopmode_to_cryo();
void save_cryostopmode();
void send_cryostopmode_to_cryo();
void load_cryostopmode();
*/

void cryo_init() {
    
    
    // Turn on the relay
    mPORTGSetBits(BIT_0);
    relay_state = true;
    save_relay_state();
    
    //Tell the cryo controller to not use the thermistor
    //command = "SET SSTOPM=0";
    // Tell cryo controller to stop if power removed from controller board. 
    // Requires the new cable
    command = "SET TSTATM=1";
    poll_cryo_session = wait_cryo_not_busy;

}

void wait_cryo_not_busy() {
    unsigned int i;
    
    if (is_cryo_busy) {
        return;
    }

    send_command_to_cryo(command, args);

    poll_recv_from_rimbox = wait_for_cryo_response;
}

void send_command_to_cryo(char *command, char *args[]) {
    unsigned int i;
    
    is_cryo_busy = true;

    strcpy(cryo_request, command);

    if (args != NULL) {
        i = 0;
        while (!(args[i] == NULL)) {
            strcat(cryo_request, " ");
            strcat(cryo_request, args[i]);
            i = i + 1;
        }
    }

    cryo_req_i = 0;

    is_cryo_response_ready = false;

    cryo_rspns_i = 0;

    start_timer(&cryo_rspns_timeout_timer, cryo_response_timeout, 700);

    poll_cryo_session = cryo_poll_send_request;
}

void wait_for_cryo_response() {
    char *rspnslns[99];
    
    unsigned int rspnsln_i = 0;

    if (!(is_cryo_response_ready)) {
        return;
    }

    rspnslns[rspnsln_i] = strtok(cryo_response, "\r");

    while (!(rspnslns[rspnsln_i] == NULL)) {
        rspnsln_i = rspnsln_i + 1;
        rspnslns[rspnsln_i] = strtok(NULL, "\r");
    }

    for (rspnsln_i = 0; rspnslns[rspnsln_i] != NULL; rspnsln_i++) {
        send_to_rimbox(rspnslns[rspnsln_i]);
        if (rspnslns[rspnsln_i + 1] != NULL) {
            send_to_rimbox(LINESEP);
        }
    }

    send_to_rimbox(EOL);

    free_cryo_session();

    poll_recv_from_rimbox = purge_chars_from_rimbox;
}

void free_cryo_session() {
    cryo_req_i = 0;

    is_cryo_busy = false;

    cryo_rspns_i = 0;

    is_cryo_response_ready = false;
}

void cryo_response_timeout() {
    strcpy(cryo_response, TIMEOUT);

    is_cryo_response_ready = true;

    poll_cryo_session = cryo_poll_idle;
}

void cryo_response_eol_timeout() {
    cryo_response[cryo_rspns_i] = 0;

    is_cryo_response_ready = true;

    poll_cryo_session = cryo_poll_idle;
}




void cryo_poll_idle() {
    if (UARTReceivedDataIsAvailable(UART2)) {
        UARTGetDataByte(UART2);
    }
}

void cryo_poll_send_request() {
    if (!(UARTTransmitterIsReady(UART2))) {
        return;
    }

    char cryo_char = cryo_request[cryo_req_i++];

    if (cryo_char == 0) {
        cryo_char = 0x0d;
        poll_cryo_session = cryo_poll_get_response_first_line;
    }

    UARTSendDataByte(UART2, cryo_char);
}

void cryo_poll_get_response_first_line() {
    if (!(UARTReceivedDataIsAvailable(UART2))) {
        return;
    }

    char cryo_char = UARTGetDataByte(UART2);

    if (((cryo_char >= 0x20) && (cryo_char <= 0x7e)) || (cryo_char == 0x0d)){
        if (cryo_rspns_i < (MAX_RESPONSE_LEN - 1)) {
            cryo_response[cryo_rspns_i] = cryo_char;
            cryo_rspns_i++;
        }
    }

    if (cryo_char == 0x0d || cryo_char == 0x0a) {
        stop_timer(&cryo_rspns_timeout_timer);
        start_timer(&cryo_rspns_eol_timer,
            cryo_response_eol_timeout, 200);
        poll_cryo_session = cryo_poll_get_response_remaining_lines;
        return;
    }
}

void cryo_poll_get_response_remaining_lines() {
    if (!(UARTReceivedDataIsAvailable(UART2))) {
        return;
    }

    char cryo_char = UARTGetDataByte(UART2);

    if (((cryo_char >= 0x20) && (cryo_char <= 0x7e)) || (cryo_char == 0x0d)){
        if (cryo_rspns_i < (MAX_RESPONSE_LEN - 1)) {
            cryo_response[cryo_rspns_i] = cryo_char;
            cryo_rspns_i++;
        }
    }
}

void getcryoattemp_command(char *args[]) {
    char msg[19];

    if (mPORTFReadBits(BIT_13)) {
        strcpy(msg, "yes");
    }
    else {
        strcpy(msg, "no");
    }

    send_to_rimbox(msg);
    send_to_rimbox(EOL);
}



/********************************************/
/********************************************/
/**              SETCRYOSTOPMODE            */
/********************************************/

/*
void setcryostopmode_command(char *args[]) {

    if (args[0] == NULL) {
        send_to_rimbox(EOL);
        return;
    }

    if (strcasecmp(args[0], "off") == 0) {
        cryostopmode = false;
        save_cryostopmode();
        send_cryostopmode_to_cryo();
        send_to_rimbox(OK);
        send_to_rimbox(EOL);
        return;
    }

    if (strcasecmp(args[0], "on") == 0) {
        cryostopmode = true;
        save_cryostopmode();
        send_cryostopmode_to_cryo();
        send_to_rimbox(OK);
        send_to_rimbox(EOL);
        return;
    }
 
    // command parameter not "off" or "on"
    send_to_rimbox(EOL);
    return;
}

void getcryostopmode_command(char *args[]) {
    char msg[19];

    if (cryostopmode == true) {
        strcpy(msg, "on");
    }
    else {
        strcpy(msg, "off");
    }

    send_to_rimbox(msg);
    send_to_rimbox(EOL);
}

void save_cryostopmode() {
    FIL fp;
    FRESULT rslt;
    UINT bytes_written;

    char text_state[9];

    strcpy(text_state, (cryostopmode == true ? "on" : "off"));

    rslt = f_open(&fp, "CRYOSTOP.TXT", (FA_CREATE_ALWAYS | FA_WRITE));
    if (rslt == FR_OK) {
        f_write(&fp, text_state, strlen(text_state), &bytes_written);
        f_close(&fp);
    }
}

void load_cryostopmode() {
    FIL fp;
    FRESULT rslt;
    UINT bytes_read;

    char text_state[19];

    cryostopmode = false;

    rslt = f_open(&fp, "CRYOSTOP.TXT", (FA_READ));
    if (rslt == FR_OK) {
        f_read(&fp, text_state, 3, &bytes_read);
        f_close(&fp);
        if (bytes_read >= 2) {
            text_state[bytes_read] = 0;
            if (strcasecmp(text_state, "on") == 0) {
                cryostopmode = true;
            }
        }
    }
}

void send_cryostopmode_to_cryo() {
    
    args[0] = NULL;
    
    if(cryostopmode == false) {
        command = "SET SSTOPM=0";
    }
    else {
        command = "SET SSTOPM=1";
    }
    poll_cryo_session = wait_cryo_not_busy;
}
*/
/*
 SET TSTATM=1
 */