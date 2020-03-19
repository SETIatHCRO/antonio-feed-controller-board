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

extern float autostart_targetTemp;
extern float autostart_highTemp;

/**
 * @file autostart_ud.h
 * @author Janusz S. Kulpa
 * @date March 2020
 * @brief File containing autostart routines - (u)p and (d)own temperature states
 *
 *
 * For bigger clarity of the state machine, the states were divided into different files
 */


int autostart_ttarget_stab_count;

//stepping temperature up and down has no difference apatr from
//the set point and "finish" function. Those two may be controlled
//by the two bool doing_XXX flags
void auto_start_u001_request()
{
    autostart_generic_cryo_request("SET MAX=240", auto_start_d001_response);
}

//setting max power
void auto_start_d001_request()
{
    autostart_machine_state |= 0x00000004;
    autostart_generic_cryo_request("SET MAX=240", auto_start_d001_response);
}

void auto_start_d001_response()
{
    autostart_generic_cryo_response(240.0, auto_start_d002_request, auto_start_e006);
}

//setting min power
void auto_start_d002_request()
{
    autostart_generic_cryo_request("SET MIN=40", auto_start_d002_response);
}

void auto_start_d002_response()
{
    autostart_generic_cryo_response(40.0, auto_start_d003_request, auto_start_e006);
}

//setting stop mode to programmable
void auto_start_d003_request()
{
    autostart_generic_cryo_request("SET SSTOPM=0", auto_start_d003_response);
}

void auto_start_d003_response()
{
    autostart_generic_cryo_response(0.0, auto_start_d004_request, auto_start_e006);
}

//starting the cooler
void auto_start_d004_request()
{
    autostart_generic_cryo_request("SET SSTOP=0", auto_start_d004_response);
}

void auto_start_d004_response()
{
    autostart_generic_cryo_response(0.0, auto_start_d005_request, auto_start_e006);
}

//setting the stop mode to relay (cooler should still be running unless 24V is down
void auto_start_d005_request()
{
    autostart_generic_cryo_request("SET SSTOPM=1", auto_start_d005_response);
}

void auto_start_d005_response()
{
    autostart_generic_cryo_response(1.0, auto_start_d006_request, auto_start_e006);
}

//Checking if the cooler is running
void auto_start_d006_request()
{
    autostart_generic_cryo_request("SET SSTOP", auto_start_d006_response);
}

void auto_start_d006_response()
{
    autostart_generic_cryo_response(0.0, auto_start_d007_request, auto_start_e006);
}

//making sure that we control taraget temperature, not cryo power
void auto_start_d007_request()
{
    autostart_generic_cryo_request("SET PID=2", auto_start_d007_response);
}

void auto_start_d007_response()
{
    autostart_generic_cryo_response(2.0, auto_start_d008_request, auto_start_e006);
}

void auto_start_d008_request()
{
    float tempk;
    tempk = auto_start_getdiode();
    if (tempk == 0.0) {
        //error
        poll_auto_start = auto_start_e007;
    }
    autostart_currtemp = tempk;
    poll_auto_start = auto_start_d008_response;
}

void auto_start_d008_response()
{
    if(doing_startup){
        if ( (autostart_currtemp >= (autostart_targetTemp - autostart_ttarget_acc)) && (autostart_currtemp <= (autostart_targetTemp + autostart_ttarget_acc)) ) {
            //we are near the target temperature during warmup
            poll_auto_start = auto_start_complete;
        } else {
            poll_auto_start = auto_start_d009_request;
        }
        return;
    }
    if(doing_shutdown){
        if (autostart_currtemp >= autostart_highTemp - autostart_ttarget_acc) {
            //tip is hot, we may shut down everything
            poll_auto_start = auto_start_s001_request;
        } else {
            poll_auto_start = auto_start_d009_request;
        }
        return;
    }
    //this should never be called here
    poll_auto_start = auto_start_e007;
}

void auto_start_d009_request()
{
    autostart_ttarget_stab_count = 30;
    if(doing_startup){
        autostart_machine_state |= 0x00000008;
        autostart_set_ttarget_withdelta_request(autostart_targetTemp,auto_start_d009_response);
        return;
    }
    if(doing_shutdown){
        autostart_machine_state |= 0x00000020;
        autostart_set_ttarget_withdelta_request(autostart_highTemp,auto_start_d009_response);
        return;
    }
    poll_auto_start = auto_start_e007;
}

void auto_start_d009_response()
{
     autostart_set_ttarget_withdelta_response(auto_start_d010_request,auto_start_e007);
}

void auto_start_d010_request()
{
    autostart_generic_vacuum_request("p302", auto_start_d010_response);
}

void auto_start_d010_response()
{
    autostart_test_vacuum_param_true(auto_start_d011_request,auto_start_e005,auto_start_e007);
}

void auto_start_d011_request()
{
    //we waited 30 min
    if(autostart_ttarget_stab_count < 1) {
        poll_auto_start = auto_start_d008_request;
    } else {
        start_timer(&auto_start_timer, auto_start_timer_callback,AUTO_START_1_MIN);
        auto_start_next_state = auto_start_d010_request;
        poll_auto_start = auto_start_delay;
    }
}
