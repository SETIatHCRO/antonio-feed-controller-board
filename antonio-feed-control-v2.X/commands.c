#ifdef __XC32
    #include <xc.h>          /* Defines special funciton registers, CP0 regs  */
#endif

#include <plib.h>           /// Include to use PIC32 peripheral libraries

#include <stdint.h>         /* For uint32_t definition                        */
#include <stdbool.h>        /* For true/false definition     */
#include <string.h>

#include "fan.h"
#include "temperature.h"
#include "vacuum.h"
#include "diode.h"
#include "accel_1.h"
#include "relay.h"
#include "rimbox.h"
#include "autostart.h"
#include "file_utils.h"
#include "voltages.h"
#include "cryo.h"
#include "internal_osc.h"
#include "rs232_test.h"
#include "commands.h"

extern char OK[];
extern char *EOL;

void restart_command();


struct command_pair commands[] = {
//TODO: this should be changed to include "guard" {NULL,NULL,NULL}
// and not use NUMBER_OF_COMMANDS
/*00*/    {"help", help_command, NULL},
/*01*/    {"setfanpwm", setfanpwm_command, "sfpwm.txt"},
/*02*/    {"getfanpwm", getfanpwm_command, "gfpwm.txt"},
/*03*/    {"getfanrpm", getfanrpm_command, "gfrpm.txt"},
/*04*/    {"getfanspeed", getfanrpm_command, "gfrpm.txt"},
/*05*/    {"gettemp", gettemp_command, "gettemp.txt"},
/*06*/    {"gt", gettemp_command, "gettemp.txt"},
/*07*/    {"getvacuum", getvacuum_command, "getvac.txt"},
/*08*/    {"getvac", getvacuum_command, "getvac.txt"},
/*09*/    {"gv", getvacuum_command, "getvac.txt"},
/*10*/    {"getdiode", getdiode_command, "getdiode.txt"},
/*11*/    {"gd", getdiode_command, "getdiode.txt"},
/*12*/    {"getaccel", getaccel_command, "getaccel.txt"},
/*13*/    {"setrelay", setrelay_command, "setrelay.txt"},
/*14*/    {"getrelay", getrelay_command, "getrelay.txt"},
/*15*/    {"ls", ls_command, "ls.txt"},
/*16*/    {"dir", ls_command, "ls.txt"},
/*17*/    {"cat", cat_command, "cat.txt"},
/*18*/    {"type", cat_command, "cat.txt"},
/*19*/    {"get24v", get24v_command, "get24v.txt"},
/*20*/    {"get48v", get48v_command, "get48v.txt"},
/*21*/    {"getcryoattemp", getcryoattemp_command, "getattmp.txt"},
/*22*/    {"setfeedstartmode", setfeedstartmode_command, "setstart.txt"},
/*23*/    {"getfeedstartmode", getfeedstartmode_command, "getstart.txt"},
/*24*/    {"hyperterminal", hyperterminal_command, "hyprterm.txt"},
/*25*/    {"ht", hyperterminal_command, "hyprterm.txt"},
/*26*/    {"minex", hyperterminal_command, "hyprterm.txt"},
/*27*/    {"stty", stty_command, "stty.txt"},
/*28*/    {"rimbox", stty_command, "stty.txt"},
/*29*/    {"getversion", getversion_command, "getversn.txt"},
/*30*/    {"bootloader", bootloader_command, "bootload.txt"},
/*31*/    {"reset", reset_command, "reset.txt"},
/*32*/    {"accelonesec", accelonesec_command, NULL},
/*33*/    {"aa", accelonesec_command, NULL},
/*34*/    {"osc", setosctun_command, NULL},
/*35*/    {"rs", rs232_test_command, NULL},
/*36*/    {"restart", restart_command, NULL},
/*37*/    {"shutdown", shutdown_command, NULL},
/*38*/    {"autostart", autostart_command, NULL},
/*39*/    {"getswitchtemp", getswitchtemp_command, NULL},
/*40*/    {"setswitchtemp", setswitchtemp_command, NULL},
/*41*/    {"getstate", autostartgetstate_command, NULL},
/*42*/    {"state", autostartgetstate_command, NULL}
    //,
   // {"setcryoprotect", setcryostopmode_command, NULL},
    //{"getcryoprotect", getcryostopmode_command, NULL}
    
};


struct command_pair *get_commands() {
    return commands;
}


void help_command(char *args[]) {
    char pathfilename[99];
    char *help_args[1];

    unsigned int cmnds_i;

    help_args[0] = pathfilename;

    if (args[0] == NULL) {
        strcpy(pathfilename, "help.txt");
        cat_command(help_args);
        return;
    }

    if (strcasecmp(args[0], "commands") == 0) {
        strcpy(pathfilename, "help/commands.txt");
        cat_command(help_args);
        return;
    }

    for (cmnds_i = 0; cmnds_i < NUMBER_OF_COMMANDS; cmnds_i++) {
        if (strcasecmp(args[0], commands[cmnds_i].name) == 0) {
            strcpy(pathfilename, "help/");
            strcat(pathfilename, commands[cmnds_i].helpfile);
            cat_command(help_args);
            return;
        }
    }

    send_to_rimbox(EOL);
}

void getfeedstartmode_command(char *args[]) {
    char msg[19];

    if (get_auto_start_state() == true) {
        strcpy(msg, "auto");
    }
    else {
        strcpy(msg, "manual");
    }

    send_to_rimbox(msg);
    send_to_rimbox(EOL);
}

void setfeedstartmode_command(char *args[]) {
    if (args[0] == NULL) {
        send_to_rimbox(EOL);
        return;
    }

    if (strcasecmp(args[0], "manual") == 0) {
        set_auto_start_state(false);
        save_autostart_state();
        send_to_rimbox(OK);
        send_to_rimbox(EOL);
        return;
    }

    if (strcasecmp(args[0], "auto") == 0) {
        set_auto_start_state(true);
        save_autostart_state();
//      poll_auto_start = auto_start_idle;
        send_to_rimbox(OK);
        send_to_rimbox(EOL);
        return;
    }

    // command parameter not "manual" or "auto"
    send_to_rimbox(EOL);
    return;
}

void restart_command() {
    /* perform a system unlock sequence */
    SYSKEY = 0xAA996655; //Unlock Sequence KEY1
    SYSKEY = 0x556699AA; //Unlock Sequence KEY2
    // SYSTEMUnlock();
    /* set SWRST bit to arm reset */
    RSWRSTSET = 1;

    /* read RSWRST register to trigger reset */
    volatile int* p = &RSWRST;
    *p;

     /* prevent any unwanted code execution until reset occurs*/
    while(1);
}
