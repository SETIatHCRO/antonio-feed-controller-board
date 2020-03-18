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
    {"help", help_command, NULL},
    {"setfanpwm", setfanpwm_command, "sfpwm.txt"},
    {"getfanpwm", getfanpwm_command, "gfpwm.txt"},
    {"getfanrpm", getfanrpm_command, "gfrpm.txt"},
    {"getfanspeed", getfanrpm_command, "gfrpm.txt"},
    {"gettemp", gettemp_command, "gettemp.txt"},
    {"gt", gettemp_command, "gettemp.txt"},
    {"getvacuum", getvacuum_command, "getvac.txt"},
    {"getvac", getvacuum_command, "getvac.txt"},
    {"gv", getvacuum_command, "getvac.txt"},
    {"getdiode", getdiode_command, "getdiode.txt"},
    {"gd", getdiode_command, "getdiode.txt"},
    {"getaccel", getaccel_command, "getaccel.txt"},
    {"setrelay", setrelay_command, "setrelay.txt"},
    {"getrelay", getrelay_command, "getrelay.txt"},
    {"ls", ls_command, "ls.txt"},
    {"dir", ls_command, "ls.txt"},
    {"cat", cat_command, "cat.txt"},
    {"type", cat_command, "cat.txt"},
    {"get24v", get24v_command, "get24v.txt"},
    {"get48v", get48v_command, "get48v.txt"},
    {"getcryoattemp", getcryoattemp_command, "getattmp.txt"},
    {"setfeedstartmode", setfeedstartmode_command, "setstart.txt"},
    {"getfeedstartmode", getfeedstartmode_command, "getstart.txt"},
    {"hyperterminal", hyperterminal_command, "hyprterm.txt"},
    {"ht", hyperterminal_command, "hyprterm.txt"},
    {"minex", hyperterminal_command, "hyprterm.txt"},
    {"stty", stty_command, "stty.txt"},
    {"rimbox", stty_command, "stty.txt"},
    {"getversion", getversion_command, "getversn.txt"},
    {"bootloader", bootloader_command, "bootload.txt"},
    {"reset", reset_command, "reset.txt"},
    {"accelonesec", accelonesec_command, NULL},
    {"aa", accelonesec_command, NULL},
    {"osc", setosctun_command, NULL},
    {"rs", rs232_test_command, NULL},
    {"restart", restart_command, NULL},
    {"shutdown", shutdown_command, NULL},
    {"autostart", autostart_command, NULL}
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
