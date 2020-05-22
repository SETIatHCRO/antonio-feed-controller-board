#include <plib.h>
#include "rimbox.h"
#include "cryo.h"
#include <stdio.h>
#include "runtime_debug.h"
#include <string.h>

#if RUNTIME_AUTOSTART_DEBUG


extern unsigned int cryo_req_i;
extern unsigned int cryo_rspns_i;
extern char cryo_request[MAX_CRYO_COMMAND_LEN];
extern char cryo_response[MAX_CRYO_RESPONSE_LEN];
char cryo_debug_response[MAX_CRYO_RESPONSE_LEN];
extern char *EOL;

void debug_command(char *args[])
{
    char msg[99];
    int m1, m2;
    snprintf(msg,98,"(%u)(%u):%s_%s\r\n",cryo_req_i,cryo_rspns_i,cryo_request,cryo_response);
    send_to_rimbox(msg);
    snprintf(msg,98,"(%x)(%x)(%x):(%x)(%x)(%x)\r\n",
            cryo_request[1],cryo_request[2],cryo_request[3],
            cryo_response[1],cryo_response[2],cryo_response[3]);
    send_to_rimbox(msg);
    m1 = UARTReceivedDataIsAvailable(UART2);
    m2 = UARTTransmitterIsReady(UART2);
    snprintf(msg,98,"%d_%d\r\n",m1,m2);
    send_to_rimbox(msg);
    strncpy(msg,cryo_debug_response,98);
    msg[98] = '\0';
    send_to_rimbox(msg);
    send_to_rimbox(EOL);
}

void debugclr_command(char *args[])
{
    //next iteration - shorten and extend the timeout

}

#endif
