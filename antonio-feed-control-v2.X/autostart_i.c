#include "autostart.h"

extern bool doing_startup;
extern bool doing_shutdown;
extern bool should_report_complete;

extern int fore_vacuum_try;
extern int turbo_power_try;

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
    should_report_complete = true;
    //resetting the try values
    turbo_power_try = 0;
    fore_vacuum_try = 0;
    autostart_generic_vacuum_request("p009=111111",auto_start_i000_response);
}

void auto_start_i000_response()
{
    autostart_generic_vacuum_response("111111", auto_start_i001_request, auto_start_e000);
}

//setting the stop mode to obey commands
void auto_start_i001_request()
{
    autostart_generic_cryo_request("SET SSTOPM=0", auto_start_i001_response);
}

void auto_start_i001_response()
{
    autostart_generic_cryo_response(0.0, auto_start_i002_request, auto_start_e000);
}

//switching off the cryo
void auto_start_i002_request()
{
    autostart_generic_cryo_request("SET SSTOP=1", auto_start_i002_response);
}

void auto_start_i002_response()
{
    autostart_generic_cryo_response(1.0, auto_start_i003_request, auto_start_e000);
}

//switching off the pumping station (to set up the parameters)
void auto_start_i003_request()
{
    autostart_generic_vacuum_request("p010=000000",auto_start_i003_response);
}

void auto_start_i003_response()
{
    autostart_generic_vacuum_response("000000", auto_start_i004_request, auto_start_e000);
}

//setting the run-up time warning to 20 minutes. That may be not necessary
//also, from now on, we should check if doing_shutdown flag is on.
//that flag is switched after calling a command from rimbox to reverse the autostart process
void auto_start_i004_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p700=000020",auto_start_i004_response);
}

void auto_start_i004_response()
{
    autostart_generic_vacuum_response("000020", auto_start_i005_request, auto_start_e000);
}

//making sure turbo pomp is(will be) off
void auto_start_i005_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p023=000000",auto_start_i005_response);
}

void auto_start_i005_response()
{
    autostart_generic_vacuum_response("000000", auto_start_i006_request, auto_start_e000);
}

//DO1 as a speed attained indicator. Not sure if necessary
void auto_start_i006_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p024=000",auto_start_i006_response);
}

void auto_start_i006_response()
{
    autostart_generic_vacuum_response("000", auto_start_i007_request, auto_start_e000);
}

//A1 as a backing pump
void auto_start_i007_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p035=003",auto_start_i007_response);
}

void auto_start_i007_response()
{
    autostart_generic_vacuum_response("003", auto_start_i008_request, auto_start_e000);
}

//backing pump in continous mode
void auto_start_i008_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p025=000",auto_start_i008_response);
}

void auto_start_i008_response()
{
    autostart_generic_vacuum_response("000", auto_start_i009_request, auto_start_e000);
}

//light gases
void auto_start_i009_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p027=001",auto_start_i009_response);
}

void auto_start_i009_response()
{
    autostart_generic_vacuum_response("001", auto_start_i010_request, auto_start_e000);
}

//Rotation speed setting mode off
void auto_start_i010_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p026=000",auto_start_i010_response);
}

void auto_start_i010_response()
{
    autostart_generic_vacuum_response("000", auto_start_i011_request, auto_start_e000);
}

//setting the standby RPM to 20% of the maximum
void auto_start_i011_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p717=002000",auto_start_i011_response);
}

void auto_start_i011_response()
{
    autostart_generic_vacuum_response("002000", auto_start_i012_request, auto_start_e000);
}

//not on standby (TODO: CHECK if that affects backing pump)
void auto_start_i012_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p002=000000",auto_start_i012_response);
}

void auto_start_i012_response()
{
    autostart_generic_vacuum_response("000000", auto_start_v001_request, auto_start_e000);
}


