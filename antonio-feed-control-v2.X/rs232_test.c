#ifdef __XC32
    #include <xc.h>          /* Defines special funciton registers, CP0 regs  */
#endif

#include <plib.h>           /// Include to use PIC32 peripheral libraries

#include <stdbool.h>        /* For true/false definition     */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "rs232_test.h"
#include "rimbox.h"
#include "user.h"

extern void toggle_watchdog();

extern char *EOL;

void rs232_test_command(char *args[]) {
    
    int i;
    char *msg = "0123456789";
    int num_lines = 1;
    
    if (args[0] != NULL) {
        num_lines = atoi(args[0]);
    }
        
    for(i = 0; i<num_lines; i++) {
        send_to_rimbox(msg);
        send_to_rimbox(EOL);
        /*
        if(i == 3) {
            send_to_rimbox("333");
            send_to_rimbox(EOL);
        }
        if(i == 6) {
            send_to_rimbox("666666");
            send_to_rimbox(EOL);
        }
        */
        toggle_watchdog();
        //if(i > 0 && (i % 100 ==0 )) delayMs(1000);
    }
    send_to_rimbox("END");
    send_to_rimbox(EOL);

}