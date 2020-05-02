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

#define MAX_NUMBER_OF_COMMANDS 100
#define NUMBER_OF_COMMANDS 49
struct command_pair *get_commands();

//#define NUMBER_OF_COMMANDS (sizeof (commands) / sizeof(struct command_pair))

#ifdef	__cplusplus
}
#endif

#endif	/* COMMANDS_H */

