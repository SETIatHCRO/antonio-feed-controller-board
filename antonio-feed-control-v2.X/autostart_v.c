#include "autostart.h"
#include <stdio.h>
#include <stdint.h>

extern bool doing_startup;
extern bool doing_shutdown;
extern bool should_report_complete;

extern char auto_start_response[AUTO_START_CMND_RSPNS_MAX_LEN];
extern int fore_vacuum_try;
extern int turbo_power_try;
extern int auto_start_cmnd_rspns_tries;
extern bool autostart_cold_start;
extern int32_t autostart_machine_state;
extern int rot_speed_test;
extern bool autostart_vac_oscilating;

#define MAX_FORE_VACUUM_TRIES 10
#define MAX_TURBO_POWER_TRIES 10

/**
 * @file autostart_v.h
 * @author Janusz S. Kulpa
 * @date March 2020
 * @brief File containing autostart routines - (v)acuum generation states
 *
 *
 * For bigger clarity of the state machine, the states were divided into different files
 */

//starting the pumping station
void auto_start_v001_request()
{
    autostart_machine_state |= 0x00000002;
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    if(autostart_cold_start)
    {
        poll_auto_start = auto_start_v007_request;
        return;
    }
    autostart_generic_vacuum_request("p010=111111",auto_start_v001_response);
}

void auto_start_v001_response()
{
    autostart_generic_vacuum_response("111111", auto_start_v002_request, auto_start_e001);
}

//making sure turbo is off and waiting 10 min
void auto_start_v002_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p023=000000",auto_start_v002_response);
}

void auto_start_v002_response()
{
    autostart_timed_vacuum_response("000000", auto_start_v004_request, auto_start_e001, AUTO_START_10_MIN);
}

//v003 was removed

//starting turbo pump
void auto_start_v004_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p023=111111",auto_start_v004_response);
}

void auto_start_v004_response()
{
    autostart_generic_vacuum_response("111111", auto_start_v005_request, auto_start_e001);
}

//turbo on stand by
void auto_start_v005_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p002=111111",auto_start_v005_response);
}

void auto_start_v005_response()
{
    autostart_timed_vacuum_response("111111", auto_start_v006_request, auto_start_e001, AUTO_START_15_MIN);
}

//switching off the standby mode and waiting 10 min
void auto_start_v006_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p002=000000",auto_start_v006_response);
}

void auto_start_v006_response()
{
    autostart_timed_vacuum_response("000000", auto_start_v007_request, auto_start_e001, AUTO_START_10_MIN);
}

//querying if turbo speed attained
void auto_start_v007_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p302",auto_start_v007_response);
}

void auto_start_v007_response()
{
    unsigned long int resp_val;
    int N;
    N = !autostart_vac_getulongfromresp(&resp_val);
    //int N = sscanf(auto_start_response, "%u", &resp_val);
    //todo:debug print
    if (N) {
        if (resp_val == 111111) {
            //if we have turbo speed attained, going forward
            poll_auto_start = auto_start_v008_request;
            return;
        } else {
            if(autostart_cold_start)
            {
                poll_auto_start = auto_start_e009;
            }
            //we havent attained the turbo speed
            fore_vacuum_try += 1;
            if (fore_vacuum_try < MAX_FORE_VACUUM_TRIES ) {
                poll_auto_start = auto_start_v005_request;
            } else {
                poll_auto_start = auto_start_e002;
            }
        }
    }
    auto_start_cmnd_rspns_tries += 1;

    if (auto_start_cmnd_rspns_tries < AUTO_START_CMND_RSPNS_MAX_TRIES) {
        poll_auto_start = auto_start_send_request_to_vac;
        return;
    }
    poll_auto_start = auto_start_e001;
}

//testing turbo power
void auto_start_v008_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p316",auto_start_v008_response);
}

void auto_start_v008_response()
{
    unsigned long int resp_val;
    int N;
    N = !autostart_vac_getulongfromresp(&resp_val);
    //int N = sscanf(auto_start_response, "%u", &resp_val);

    if (N) {
        if (resp_val <= 20) {
            //if we have low turbo power (good vacuum)
            rot_speed_test = 0;
            poll_auto_start = auto_start_v009_request;
            return;
        } else {
            //we havent attained the turbo power
            if(autostart_cold_start)
            {
                poll_auto_start = auto_start_e009;
            }
            turbo_power_try += 1;
            if (turbo_power_try < MAX_TURBO_POWER_TRIES ) {
                poll_auto_start = auto_start_v005_request;
            } else {
                poll_auto_start = auto_start_e003;
            }
        }
    }
    auto_start_cmnd_rspns_tries += 1;

    if (auto_start_cmnd_rspns_tries < AUTO_START_CMND_RSPNS_MAX_TRIES) {
        poll_auto_start = auto_start_send_request_to_vac;
        return;
    }
    poll_auto_start = auto_start_e001;
}

//testing turbo power
void auto_start_v009_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p398",auto_start_v009_response);
}

void auto_start_v009_response()
{
    unsigned long int resp_val;
    int N;
    N = !autostart_vac_getulongfromresp(&resp_val);
    //int N = sscanf(auto_start_response, "%u", &resp_val);

    if (N) {
        if (resp_val > 90100) {
            //turbo speed too big, probably oscilation mode
            //we need to reset turbo (or if we already did that
            //there is bigger problem
            if (autostart_vac_oscilating) {
                poll_auto_start = auto_start_e010;
            }
            autostart_vac_oscilating = true;
            poll_auto_start = auto_start_v010_request;
            return;
        } else {
            rot_speed_test++;
            //if tested 10 times if turbo speed is ok.
            //we may proceed further, if not, test again
            if (rot_speed_test > 10)
            {
                poll_auto_start = auto_start_d001_request;
                return;
            }
            else
            {
                poll_auto_start = auto_start_v009_request;
                return;
            }
        }
    }
    auto_start_cmnd_rspns_tries += 1;

    if (auto_start_cmnd_rspns_tries < AUTO_START_CMND_RSPNS_MAX_TRIES) {
        poll_auto_start = auto_start_send_request_to_vac;
        return;
    }
    poll_auto_start = auto_start_e001;
}

//making sure turbo is off and waiting 1 min
void auto_start_v010_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p023=000000",auto_start_v010_response);
}

void auto_start_v010_response()
{
    autostart_timed_vacuum_response("000000", auto_start_v011_request, auto_start_e001, AUTO_START_1_MIN);
}

//making sure turbo is on and waiting 1 min
void auto_start_v011_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p023=111111",auto_start_v011_response);
}

void auto_start_v011_response()
{
    autostart_timed_vacuum_response("111111", auto_start_v007_request, auto_start_e001, AUTO_START_1_MIN);
}
