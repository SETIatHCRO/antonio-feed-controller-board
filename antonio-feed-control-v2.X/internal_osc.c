


#ifdef __XC32
    #include <xc.h>          /* Defines special funciton registers, CP0 regs  */
#endif

#include <plib.h>           /// Include to use PIC32 peripheral libraries

#include <stdbool.h>        /* For true/false definition     */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "internal_osc.h"
#include "rimbox.h"

extern char *EOL;

void init_osctun() {
    SYSKEY = 0xAA996655; //OSCTUN Unlock Sequence KEY1
    SYSKEY = 0x556699AA; //OSCTUN Unlock Sequence KEY2
    OSCTUN = 0x0000;
    SYSKEY = 0x12345678; //Lock Syskey, Write any value other than Key1 or Key2
}

void set_osctun(int value) {
    SYSKEY = 0xAA996655; //OSCTUN Unlock Sequence KEY1
    SYSKEY = 0x556699AA; //OSCTUN Unlock Sequence KEY2
    OSCTUN = value;
    SYSKEY = 0x12345678; //Lock Syskey, Write any value other than Key1 or Key2
}

void setosctun_command(char *args[]) {
    char msg[16];
    float temp;
   
    if (args[0] == NULL) {
        send_to_rimbox(EOL);
        return;
    }
    
    set_osctun(atoi(args[0]));
    
    sprintf(msg, "OSCTUN %d", atoi(args[0]));
    send_to_rimbox(msg);
    send_to_rimbox(EOL);
    
    return;
}