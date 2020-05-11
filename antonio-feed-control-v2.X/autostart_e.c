#include "autostart.h"
#include "rimbox.h"
#include "file_utils.h"
#include <stdbool.h>
#include <stdint.h>
#include <plib.h>           /* Include to use PIC32 peripheral libraries      */

extern int32_t autostart_machine_state;
extern bool doing_startup;
extern bool doing_shutdown;
extern bool relay_state;
extern bool update_logs;
extern bool error_shutdown;

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
    autostart_machine_state |= 0x00000100;
    doing_startup = false;
    update_logs=true;
    feedlog_always("e000");
    send_to_rimbox("\r\nautostart error 000\r\n");
    send_to_rimbox("init routines failed\r\n");
    send_to_rimbox("shutting down\r\n");
    error_shutdown = true;
    poll_auto_start = auto_start_s001_request;
}

void auto_start_e001()
{
    autostart_machine_state |= 0x00000200;
    doing_startup = false;
    update_logs=true;
    feedlog_always("e001");
    send_to_rimbox("\r\nautostart error 001\r\n");
    send_to_rimbox("vacuum routines failed\r\n");
    send_to_rimbox("shutting down\r\n");
    error_shutdown = true;
    poll_auto_start = auto_start_s001_request;
}

void auto_start_e002()
{
    autostart_machine_state |= 0x00000400;
    doing_startup = false;
    update_logs=true;
    feedlog_always("e002");
    send_to_rimbox("\r\nautostart error 002\r\n");
    send_to_rimbox("turbo RPM not attained\r\n");
    send_to_rimbox("shutting down\r\n");
    error_shutdown = true;
    poll_auto_start = auto_start_s001_request;
}

void auto_start_e003()
{
    autostart_machine_state |= 0x00000800;
    doing_startup = false;
    update_logs=true;
    feedlog_always("e003");
    send_to_rimbox("\r\nautostart error 003\r\n");
    send_to_rimbox("turbo low power not attained\r\n");
    send_to_rimbox("shutting down\r\n");
    error_shutdown = true;
    poll_auto_start = auto_start_s001_request;
}

void auto_start_e004()
{
    autostart_machine_state |= 0x00001000;
    update_logs=true;
    feedlog_always("e004");
    feedlog("Serious error during shutdown 004");
    send_to_rimbox("\r\nautostart error\r\n");
    send_to_rimbox("error during shutting down\r\n");
    relay_state = false;
    mPORTGClearBits(BIT_0);
    poll_auto_start = auto_start_error;
}

void auto_start_e005()
{
    autostart_machine_state |= 0x00002000;
    doing_startup = false;
    doing_shutdown = false;
    update_logs=true;
    feedlog_always("e005");
    send_to_rimbox("\r\ncryo/turbo\r\n");
    send_to_rimbox("turbo low rpm while cryo on\r\n");
    send_to_rimbox("shutting down\r\n");
    error_shutdown = true;
    poll_auto_start = auto_start_s001_request;
}

void auto_start_e006()
{
    autostart_machine_state |= 0x00004000;
    doing_startup = false;
    update_logs=true;
    feedlog_always("e006");
    send_to_rimbox("\r\nautostart error 006\r\n");
    send_to_rimbox("cryo setup failed\r\n");
    send_to_rimbox("shutting down\r\n");
    error_shutdown = true;
    poll_auto_start = auto_start_s001_request;
}

void auto_start_e007()
{
    autostart_machine_state |= 0x00008000;
    doing_startup = false;
    doing_shutdown = true;
    update_logs=true;
    feedlog_always("e007");
    send_to_rimbox("\r\nautostart error 007\r\n");
    send_to_rimbox("cryo cooling routines failed\r\n");
    send_to_rimbox("trying to heat up\r\n");
    poll_auto_start = auto_start_u001_request;
}

void auto_start_e008()
{
    autostart_machine_state |= 0x00010000;
    doing_startup = false;
    doing_shutdown = true;
    update_logs=true;
    feedlog_always("e008");
    send_to_rimbox("\r\nautostart error 008\r\n");
    send_to_rimbox("error during cold start init\r\n");
    poll_auto_start = auto_start_u001_request;
}

void auto_start_e009()
{
    autostart_machine_state |= 0x00020000;
    doing_startup = false;
    doing_shutdown = true;
    update_logs=true;
    feedlog_always("e009");
    send_to_rimbox("\r\nautostart error 009\r\n");
    send_to_rimbox("vac error during cold start\r\n");
    poll_auto_start = auto_start_u001_request;
}

void auto_start_e010()
{
    autostart_machine_state |= 0x00040000;
    doing_startup = false;
    doing_shutdown = true;
    update_logs=true;
    feedlog_always("e010");
    send_to_rimbox("\r\nautostart error 010\r\n");
    send_to_rimbox("vac oscilation error\r\n");
    poll_auto_start = auto_start_u001_request;
}

void auto_start_e011()
{
    autostart_machine_state |= 0x00080000;
    doing_startup = false;
    doing_shutdown = true;
    update_logs=true;
    feedlog_always("e011");
    send_to_rimbox("\r\nautostart error 011\r\n");
    send_to_rimbox("cryo temp\r\n");
    poll_auto_start = auto_start_u001_request;
}

void auto_start_e012()
{
    autostart_machine_state |= 0x00200000;
    update_logs=true;
    feedlog_always("e012");
    feedlog("cryo error e012");
    send_to_rimbox("error during cold\r\n");
    relay_state = false;
    mPORTGClearBits(BIT_0);
    poll_auto_start = auto_start_error;
}
