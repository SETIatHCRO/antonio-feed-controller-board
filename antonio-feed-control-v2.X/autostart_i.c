#include "autostart.h"
#include <stdint.h>

extern int32_t autostart_machine_state;
extern bool doing_startup;
extern bool doing_shutdown;
extern bool should_report_complete;
extern float autostart_coold_start_threshold;
extern int fore_vacuum_try;
extern int turbo_power_try;
extern bool autostart_cold_start;
extern bool autostart_vac_oscilating;
extern unsigned int auto_start_cmnd_rspns_tries;


/**
 * @file autostart_i.h
 * @author Janusz S. Kulpa
 * @date March 2020
 * @brief File containing autostart routines - (i)nitialization states
 *
 * In the initialization step, the main purpose is to make sure that the cryo pump
 * is off and various turbo pomp settings are provided properly.
 * Each step consist of request and response. Starting from i004 request, it's possible to
 * terminate the process and move to shutdown routine (s001)
 * Since we are not yet starting a cooling process, each step is linked to the same
 * error response e000
 *
 * For bigger clarity of the state machine, the states were divided into different files
 */

//enabling errors
void auto_start_i000_request()
{
    autostart_machine_state |= 0x00000001;
    should_report_complete = true;
    //resetting the try values
    turbo_power_try = 0;
    fore_vacuum_try = 0;
    autostart_cold_start = false;
    autostart_vac_oscilating = false;

    autostart_generic_vacuum_request("p009=111111",auto_start_i000_response);
}

void auto_start_i000_response()
{
    autostart_generic_vacuum_response("111111", auto_start_i001_request, auto_start_e000);
}

void auto_start_i001_request()
{
    float tempk;
    tempk = auto_start_getdiode();
    if (tempk == 0.0) {
        //error
        poll_auto_start = auto_start_e000;
    }
    if (tempk < autostart_coold_start_threshold)
    {
        poll_auto_start = auto_start_i002b_request;
    } else {
        poll_auto_start = auto_start_i002a_request;
    }
}

//setting the stop mode to obey commands
void auto_start_i002a_request()
{
    doing_startup = true;
    autostart_generic_cryo_request("SET SSTOPM=0", auto_start_i002a_response);
}

void auto_start_i002a_response()
{
    autostart_generic_cryo_response(0.0, auto_start_i003a_request, auto_start_e000);
}

//switching off the cryo
void auto_start_i003a_request()
{
    autostart_generic_cryo_request("SET SSTOP=1", auto_start_i003a_response);
}

void auto_start_i003a_response()
{
    autostart_generic_cryo_response(1.0, auto_start_i004a_request, auto_start_e000);
}

//switching off the pumping station (to set up the parameters)
void auto_start_i004a_request()
{
    autostart_generic_vacuum_request("p010=000000",auto_start_i004a_response);
}

void auto_start_i004a_response()
{
    autostart_generic_vacuum_response("000000", auto_start_i005a_request, auto_start_e000);
}

//setting the run-up time warning to 20 minutes. That may be not necessary
//also, from now on, we should check if doing_shutdown flag is on.
//that flag is switched after calling a command from rimbox to reverse the autostart process
void auto_start_i005a_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p700=000020",auto_start_i005a_response);
}

void auto_start_i005a_response()
{
    autostart_generic_vacuum_response("000020", auto_start_i006a_request, auto_start_e000);
}

//making sure turbo pomp is(will be) off
void auto_start_i006a_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p023=000000",auto_start_i006a_response);
}

void auto_start_i006a_response()
{
    autostart_generic_vacuum_response("000000", auto_start_i007_request, auto_start_e000);
}

//DO1 as a speed attained indicator. Not sure if necessary
void auto_start_i007_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p024=000",auto_start_i007_response);
}

void auto_start_i007_response()
{
    autostart_generic_vacuum_response("000", auto_start_i008_request, auto_start_e000);
}

//A1 as a backing pump
void auto_start_i008_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p035=003",auto_start_i008_response);
}

void auto_start_i008_response()
{
    autostart_generic_vacuum_response("003", auto_start_i009_request, auto_start_e000);
}

//backing pump in continous mode
void auto_start_i009_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p025=000",auto_start_i009_response);
}

void auto_start_i009_response()
{
    autostart_generic_vacuum_response("000", auto_start_i010_request, auto_start_e000);
}

//light gases
void auto_start_i010_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p027=001",auto_start_i010_response);
}

void auto_start_i010_response()
{
    autostart_generic_vacuum_response("001", auto_start_i011_request, auto_start_e000);
}

//Rotation speed setting mode off
void auto_start_i011_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p026=000",auto_start_i011_response);
}

void auto_start_i011_response()
{
    autostart_generic_vacuum_response("000", auto_start_i012_request, auto_start_e000);
}

//setting the standby RPM to 20% of the maximum
void auto_start_i012_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p717=002000",auto_start_i012_response);
}

void auto_start_i012_response()
{
    autostart_generic_vacuum_response("002000", auto_start_i013_request, auto_start_e000);
}

//not on standby (TODO: CHECK if that affects backing pump)
//and that may not be necessary here if that doesn't affect backing pump
void auto_start_i013_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p002=000000",auto_start_i013_response);
}

void auto_start_i013_response()
{
    autostart_generic_vacuum_response("000000", auto_start_i014_request, auto_start_e000);
}

void auto_start_i014_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p701=000075",auto_start_i014_response);
}

void auto_start_i014_response()
{
    autostart_generic_vacuum_response("000075", auto_start_v001_request, auto_start_e000);
}

//setting the stop mode to obey commands
void auto_start_i002b_request()
{
    autostart_generic_cryo_request("SET SSTOPM=0", auto_start_i002b_response);
}

void auto_start_i002b_response()
{
    autostart_generic_cryo_response(0.0, auto_start_i003b_request, auto_start_e008);
}

//make sure pumping station is on
void auto_start_i003b_request()
{
    autostart_generic_vacuum_request("p010=111111",auto_start_i003b_response);
}

void auto_start_i003b_response()
{
    autostart_generic_vacuum_response("111111", auto_start_i004b_request, auto_start_e008);
}

//make sure turbo is on
void auto_start_i004b_request()
{
    autostart_generic_vacuum_request("p023=111111",auto_start_i004b_response);
}

void auto_start_i004b_response()
{
    autostart_generic_vacuum_response("111111", auto_start_i005b_request, auto_start_e008);
}

//make sure not on standby
void auto_start_i005b_request()
{
    autostart_generic_vacuum_request("p002=000000",auto_start_i005b_response);
}

void auto_start_i005b_response()
{
    autostart_generic_vacuum_response("000000", auto_start_i006b_request, auto_start_e008);
}

//test cryo state
void auto_start_i006b_request()
{
    autostart_generic_cryo_request("SET SSTOP", auto_start_i006b_response);
}

void auto_start_i006b_response()
{
    float cryo_flt_rspns;

    int Ncr = 0;
    Ncr = !autostart_cryo_getfloatfromresp(&cryo_flt_rspns);

    if ((Ncr) && (cryo_flt_rspns > -0.1) && (cryo_flt_rspns <  0.1)) {
        autostart_cold_start = true;
        doing_startup = true
        poll_auto_start = auto_start_i007_request;
        return;
    }
    if ((Ncr) && (cryo_flt_rspns > -1.1) && (cryo_flt_rspns <  1.1)) {
        poll_auto_start = auto_start_u001_request;
        return;
    }
    auto_start_cmnd_rspns_tries += 1;

    if (auto_start_cmnd_rspns_tries < AUTO_START_CMND_RSPNS_MAX_TRIES) {
        poll_auto_start = auto_start_send_request_to_cryo;
        return;
    }

    poll_auto_start = auto_start_e008;
}




