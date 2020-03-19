#include "autostart.h"
#include <stdint.h>

extern int32_t autostart_machine_state;
extern bool doing_startup;
extern bool doing_shutdown;
extern bool should_report_complete;

/**
 * @file autostart_s.h
 * @author Janusz S. Kulpa
 * @date March 2020
 * @brief File containing autostart routines - (s)hutdown states
 *
 * the final states of the shutdown mechanism
 *
 * For bigger clarity of the state machine, the states were divided into different files
 */

//setting the stop mode to obey commands
void auto_start_s001_request()
{
    autostart_machine_state |= 0x00000040;
    autostart_generic_cryo_request("SET SSTOPM=0", auto_start_s001_response);
}

void auto_start_s001_response()
{
    autostart_generic_cryo_response(0.0, auto_start_s002_request, auto_start_e004);
}

//switching off the cryo
void auto_start_s002_request()
{
    autostart_generic_cryo_request("SET SSTOP=1", auto_start_s002_response);
}

void auto_start_s002_response()
{
    autostart_generic_cryo_response(1.0, auto_start_s003_request, auto_start_e004);
}

//switching off the pumping station
void auto_start_s003_request()
{
    autostart_generic_vacuum_request("p010=000000",auto_start_s003_response);
}

void auto_start_s003_response()
{
    autostart_generic_vacuum_response("000000", shutdown_complete, auto_start_e004);
}
