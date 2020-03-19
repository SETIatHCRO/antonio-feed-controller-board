#include "autostart.h"
#include <stdio.h>

extern bool doing_startup;
extern bool doing_shutdown;
extern bool should_report_complete;

extern char auto_start_response[AUTO_START_CMND_RSPNS_MAX_LEN];
extern int auto_start_cmnd_rspns_tries;


/**
 * @file autostart_ud.h
 * @author Janusz S. Kulpa
 * @date March 2020
 * @brief File containing autostart routines - (u)p and (d)own temperature states
 *
 *
 * For bigger clarity of the state machine, the states were divided into different files
 */
