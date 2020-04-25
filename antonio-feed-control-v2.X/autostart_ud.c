#include "autostart.h"
#include "oneshot.h"
#include <stdio.h>
#include <stdint.h>

extern int32_t autostart_machine_state;
extern bool doing_startup;
extern bool doing_shutdown;
extern bool should_report_complete;

extern char auto_start_request[AUTO_START_CMND_RSPNS_MAX_LEN];
extern char auto_start_response[AUTO_START_CMND_RSPNS_MAX_LEN];
extern int auto_start_cmnd_rspns_tries;

extern float autostart_currtemp;
extern float autostart_ttarget_acc;
extern float autostart_30min_delta_temp;
extern struct oneshot_timer auto_start_timer;

extern float autostart_highTemp;
extern float autostart_switchTemp;

/**
 * @file autostart_ud.h
 * @author Janusz S. Kulpa
 * @date March 2020
 * @brief File containing autostart routines - (u)p and (d)own temperature states
 *
 *
 * For bigger clarity of the state machine, the states were divided into different files
 */


//making sure that we control cryo power
void auto_start_d001_request()
{
    autostart_machine_state |= 0x00000004;
    autostart_generic_cryo_request("SET PID=1", auto_start_d001_response);
}

void auto_start_d001_response()
{
    autostart_generic_cryo_response(1.0, auto_start_d002_request, auto_start_e006);
}

//cryo power to 70W
void auto_start_d002_request()
{
    autostart_generic_cryo_request("SET PWOUT=70", auto_start_d002_response);
}

void auto_start_d002_response()
{
    autostart_generic_cryo_response(70.0, auto_start_d003_request, auto_start_e006);
}


//starting the cooler
void auto_start_d003_request()
{
    autostart_machine_state |= 0x00000008;
    autostart_generic_cryo_request("SET SSTOP=0", auto_start_d003_response);
}

void auto_start_d003_response()
{
    autostart_generic_cryo_response(0.0, auto_start_d004_request, auto_start_e006);
}


void auto_start_d004_request()
{
    autostart_generic_vacuum_request("p302", auto_start_d004_response);
}

void auto_start_d004_response()
{
    autostart_test_vacuum_param_true(auto_start_d005_request,auto_start_e005,auto_start_e007);
}

void auto_start_d005_request()
{
    float tempk;
    tempk = auto_start_getdiode();
    if (tempk == 0.0) {
        //error
        poll_auto_start = auto_start_e007;
    }
    if(doing_shutdown) {
        poll_auto_start = auto_start_u001_request;
        return;
    }
    if (tempk < autostart_switchTemp) {
        poll_auto_start = auto_start_d006_request;
        return;
    } else {
         start_timer(&auto_start_timer, auto_start_timer_callback,AUTO_START_5_MIN);
         auto_start_next_state = auto_start_d004_request;
         poll_auto_start = auto_start_delay;
         return;
    }
}

//starting the cooler
void auto_start_d006_request()
{
    autostart_machine_state |= 0x00000010;
    autostart_generic_cryo_request("SET PID=2", auto_start_d006_response);
}

void auto_start_d006_response()
{
    autostart_generic_cryo_response(2.0, auto_start_complete, auto_start_e006);
}

//UP

//starting the cooler
void auto_start_u001_request()
{
    autostart_machine_state |= 0x00000020;
    autostart_generic_cryo_request("SET SSTOP=1", auto_start_u001_response);
}

void auto_start_u001_response()
{
    autostart_generic_cryo_response(1.0, auto_start_u002_request, auto_start_e004);
}

void auto_start_u002_request()
{
    float tempk;
    tempk = auto_start_getdiode();
    if (tempk == 0.0) {
        //error
        poll_auto_start = auto_start_e007;
    }
    if(doing_startup) {
        poll_auto_start = auto_start_d001_request;
        return;
    }
    if (tempk > autostart_highTemp) {
        poll_auto_start = auto_start_s001_request;
        return;
    } else {
         start_timer(&auto_start_timer, auto_start_timer_callback,AUTO_START_5_MIN);
         auto_start_next_state = auto_start_u002_request;
         poll_auto_start = auto_start_delay;
         return;
    }
}
