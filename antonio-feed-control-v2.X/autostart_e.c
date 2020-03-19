#include "autostart.h"
#include "rimbox.h"
#include <stdbool.h>

extern bool doing_startup;
extern bool doing_shutdown;

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
    doing_startup = false;
    send_to_rimbox("\r\nautostart error\r\n");
    send_to_rimbox("init routines failed\r\n");
    send_to_rimbox("shutting down\r\n");
    poll_auto_start = auto_start_s001_request;
}

void auto_start_e001()
{
    doing_startup = false;
    send_to_rimbox("\r\nautostart error\r\n");
    send_to_rimbox("vacuum routines failed\r\n");
    send_to_rimbox("shutting down\r\n");
    poll_auto_start = auto_start_s001_request;
}

void auto_start_e002()
{
    doing_startup = false;
    send_to_rimbox("\r\nautostart error\r\n");
    send_to_rimbox("turbo RPM not attained\r\n");
    send_to_rimbox("shutting down\r\n");
    poll_auto_start = auto_start_s001_request;
}

void auto_start_e003()
{
    doing_startup = false;
    send_to_rimbox("\r\nautostart error\r\n");
    send_to_rimbox("turbo low power not attained\r\n");
    send_to_rimbox("shutting down\r\n");
    poll_auto_start = auto_start_s001_request;
}

void auto_start_e004()
{
    feedlog("Serious error during shutdown")
    send_to_rimbox("\r\nautostart error\r\n");
    send_to_rimbox("turbo low power not attained\r\n");
    send_to_rimbox("shutting down\r\n");
    poll_auto_start = auto_start_error;
}

void auto_start_e005()
{
    feedlog("turbo on low RMP while cryo is on");
    doing_startup = false;
    doing_shutdown = false;
    send_to_rimbox("\r\ncryo/turbo\r\n");
    send_to_rimbox("turbo low rpm while cryo on\r\n");
    send_to_rimbox("shutting down\r\n");
    poll_auto_start = auto_start_s001_request;
}
