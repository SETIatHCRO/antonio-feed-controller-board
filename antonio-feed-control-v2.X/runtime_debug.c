#include "rimbox.h"
#include "cryo.h"
#include <stdio.h>
#include "runtime_debug.h"

#if RUNTIME_AUTOSTART_DEBUG


extern unsigned int cryo_req_i;
extern unsigned int cryo_rspns_i;
extern char cryo_request[MAX_CRYO_COMMAND_LEN];
extern char cryo_response[MAX_CRYO_RESPONSE_LEN];
extern char *EOL;

void debug_command(char *args[])
{
    char msg[99];
    snprintf(msg,98,"(%u)(%u):%s_%s",cryo_req_i,cryo_rspns_i,cryo_request,cryo_response);
    send_to_rimbox(msg);
    send_to_rimbox(EOL);
}

void debugclr_command(char *args[])
{

}

#endif
