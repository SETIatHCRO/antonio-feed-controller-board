#include <plib.h>
#include "rimbox.h"
#include "cryo.h"
#include <stdio.h>
#include "runtime_debug.h"
#include <string.h>

#if RUNTIME_AUTOSTART_DEBUG


extern unsigned int cryo_req_i;
extern unsigned int cryo_rspns_i;
unsigned int last_chr_dbg_req=0;
unsigned int last_chr_dbg_resp=0;
extern char cryo_request[MAX_CRYO_COMMAND_LEN];
extern char cryo_response[MAX_CRYO_RESPONSE_LEN];
extern char *EOL;

unsigned int debug_vac_overflow_cnt = 0;
unsigned int debug_cryo_overflow_cnt = 0;

void debug_command(char *args[])
{
    char msg[99];
    int m1, m2;
    unsigned int s1,s2;
    snprintf(msg,98,"(%u)(%u):%s_%s\r\n",cryo_req_i,cryo_rspns_i,cryo_request,cryo_response);
    send_to_rimbox(msg);
    m1 = UARTReceivedDataIsAvailable(UART2);
    m2 = UARTTransmitterIsReady(UART2);
    snprintf(msg,98,"%d_%d_(%u_%u)\r\n",m1,m2,last_chr_dbg_req, last_chr_dbg_resp);
    send_to_rimbox(msg);
    snprintf(msg,98,"vof: %u cof: %u\r\n",debug_vac_overflow_cnt,debug_cryo_overflow_cnt);
    send_to_rimbox(msg);
    s1 = U2STA;
    s2 = U3STA;
    snprintf(msg,98,"v_sta: 0x%x c_sta: 0x%x\r\n",s2,s1);
    send_to_rimbox(msg);
    s1 = U2TXREG;
    s2 = U3TXREG;
    snprintf(msg,98,"v_txreg: 0x%x c_txreg: 0x%x\r\n",s2,s1);
    send_to_rimbox(msg);
    send_to_rimbox(EOL);
}

void debugclr_command(char *args[])
{
    //next iteration - shorten and extend the timeout
    debug_vac_overflow_cnt = 0;
    debug_cryo_overflow_cnt = 0;
}

#endif
