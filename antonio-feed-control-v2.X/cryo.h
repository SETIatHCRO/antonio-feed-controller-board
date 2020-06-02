/* 
 * File:   cryo.h
 * Author: Jon's Windows
 *
 * Created on January 7, 2019, 9:21 AM
 */

#ifndef CRYO_H
#define	CRYO_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#define MAX_CRYO_COMMAND_LEN 999
#define MAX_CRYO_RESPONSE_LEN 999
#define MAX_CRYO_RESPONSE_LINES 99

void (*poll_cryo_session)();

void cryo_init();
int cryo_test_uart_err();
void wait_cryo_not_busy();

void send_command_to_cryo(char *command, char *args[]);
void free_cryo_session();
void cryo_response_timeout();
void cryo_response_eol_timeout();
void cryo_poll_idle();
void cryo_poll_send_request();
void cryo_poll_get_response_first_line();
void cryo_poll_get_response_remaining_lines();
void getcryoattemp_command(char *args[]);

void setcryostopmode_command(char *args[]);
void getcryostopmode_command(char *args[]);



#ifdef	__cplusplus
}
#endif

#endif	/* CRYO_H */

