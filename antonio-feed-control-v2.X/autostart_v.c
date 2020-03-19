#include "autostart.h"
#include <stdio.h>

extern bool doing_startup;
extern bool doing_shutdown;
extern bool should_report_complete;

extern char auto_start_response[AUTO_START_CMND_RSPNS_MAX_LEN];
extern int fore_vacuum_try;
extern int turbo_power_try;
extern int auto_start_cmnd_rspns_tries;

#define MAX_FORE_VACUUM_TRIES 3
#define MAX_TURBO_POWER_TRIES 3

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
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p010=111111",auto_start_v001_response);
}

void auto_start_v001_response()
{
    autostart_generic_vacuum_response("111111", auto_start_v002_request, auto_start_e001);
}

//making sure turbo is off and waiting 30 min
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
    autostart_timed_vacuum_response("000000", auto_start_v003_request, auto_start_e001, AUTO_START_30_MIN);
}

//rot speed switch point (indicated by p316) setting to 75%
void auto_start_v003_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p701=000075",auto_start_v003_response);
}

void auto_start_v003_response()
{
    autostart_generic_vacuum_response("000075", auto_start_v004_request, auto_start_e001);
}

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
    autostart_timed_vacuum_response("111111", auto_start_v006_request, auto_start_e001, AUTO_START_30_MIN);
}

//switching off the standby mode and waiting 15 min
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
    autostart_timed_vacuum_response("000000", auto_start_v007_request, auto_start_e001, AUTO_START_15_MIN);
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
    unsigned int resp_val;
    int N = sscanf(auto_start_response, "%u", &resp_val);

    if (N==1) {
        if (resp_val == 111111) {
            //if we have turbo speed attained, going forward
            poll_auto_start = auto_start_v008_request;
            return;
        } else {
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
    unsigned int resp_val;
    int N = sscanf(auto_start_response, "%u", &resp_val);

    if (N==1) {
        if (resp_val <= 20) {
            //if we have low turbo power (good vacuum)
            poll_auto_start = auto_start_d001_request;
            return;
        } else {
            //we havent attained the turbo power
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
