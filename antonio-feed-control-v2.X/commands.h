/* 
 * File:   commands.h
 * Author: Jon's Windows
 *
 * Created on January 7, 2019, 10:01 AM
 */

#ifndef COMMANDS_H
#define	COMMANDS_H

/*
#include <stdio.h>

#include "rimbox.h"
#include "fan.h"
#include "relay.h"
#include "cryo.h"
#include "accel_1.h"
#include "diode.h"
#include "temperature.h"
#include "file_utils.h"
#include "voltages.h"
#include "vacuum.h"
*/

#ifdef	__cplusplus
extern "C" {
#endif

typedef void (*command_function) (char *args[]);

struct command_pair {
    char *name;
    command_function function;
    char *helpfile;
};

void help_command(char *args[]);
void bootloader_command(char *args[]);
void getversion_command(char *args[]);
void getfeedstartmode_command(char *args[]);
void setfeedstartmode_command(char *args[]);

/*
 * associate command names with their functions and help files
 */
/*
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
    {"reset", reset_command, "reset.txt"}
};
*/

#define NUMBER_OF_COMMANDS 34
struct command_pair *get_commands();

//#define NUMBER_OF_COMMANDS (sizeof (commands) / sizeof(struct command_pair))

#ifdef	__cplusplus
}
#endif

#endif	/* COMMANDS_H */

