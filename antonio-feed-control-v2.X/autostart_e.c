#include "autostart.h"

/**
 * @file autostart_e.h
 * @author Janusz S. Kulpa
 * @date March 2020
 * @brief File containing autostart routines - (e)rror states
 *
 *
 * For bigger clarity of the state machine, the states were divided into different files
 */

void auto_start_e000()
{
    send_to_rimbox("\r\nautostart error\r\n");
    send_to_rimbox("init routines failed\r\n");
    send_to_rimbox("shutting down\r\n");
    poll_auto_start = auto_start_s001_request;
}

void auto_start_e001()
{
    send_to_rimbox("\r\nautostart error\r\n");
    send_to_rimbox("vacuum routines failed\r\n");
    send_to_rimbox("shutting down\r\n");
    poll_auto_start = auto_start_s001_request;
}

void auto_start_e002()
{
    send_to_rimbox("\r\nautostart error\r\n");
    send_to_rimbox("turbo RPM not attained\r\n");
    send_to_rimbox("shutting down\r\n");
    poll_auto_start = auto_start_s001_request;
}

void auto_start_e003()
{
    send_to_rimbox("\r\nautostart error\r\n");
    send_to_rimbox("turbo low power not attained\r\n");
    send_to_rimbox("shutting down\r\n");
    poll_auto_start = auto_start_s001_request;
}
