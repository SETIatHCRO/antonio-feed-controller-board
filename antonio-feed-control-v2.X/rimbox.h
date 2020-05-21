/* 
 * File:   rimbox.h
 * Author: Jon's Windows
 *
 * Created on January 7, 2019, 9:16 AM
 */

#ifndef RIMBOX_H
#define	RIMBOX_H

#ifdef	__cplusplus
extern "C" {
#endif

#define MAX_ARGS 99
#define MAX_RBOX_COMMAND_LEN 999

void wait_for_cryo_response();

void rimbox_poll_idle();
void purge_chars_from_rimbox();
void recv_cmnd_from_rimbox();
void poll_send_to_rimbox();
void send_to_rimbox(char *msg);
void send_char_to_rimbox(char c);
void parse_cmnd_from_rimbox();
void reset_command(char *args[]);
void (*poll_recv_from_rimbox)();


#ifdef	__cplusplus
}
#endif

#endif	/* RIMBOX_H */

