#include "autostart.h"

extern bool doing_startup;
extern bool doing_shutdown;
extern bool should_report_complete;

void auto_start_i000_request()
{
    should_report_complete = true;
    autostart_generic_vacuum_request("p009=111111",auto_start_i000_response);
}

void auto_start_i000_response()
{
    autostart_generic_vacuum_response("111111", auto_start_i001_request, auto_start_e000);
}

void auto_start_i004_request()
{
    if(doing_shutdown) {
        poll_auto_start = auto_start_s001_request;
        return;
    }
    autostart_generic_vacuum_request("p700=000020",auto_start_i004_response);
}

