/* 
 * File:   vacuum.h
 * Author: Jon's Windows
 *
 * Created on January 7, 2019, 10:56 AM
 */

#ifndef VACUUM_H
#define	VACUUM_H

#ifdef	__cplusplus
extern "C" {
#endif

#define MAX_VAC_COMMAND_LEN 99
#define MAX_VAC_RESPONSE_LEN 99

void getvacuum_command(char *args[]);
void wait_vacuum_not_busy();
void send_command_to_vacuum(char *command);
void wait_for_vacuum_response();
void free_vacuum_session();
void parse_vacuum_response(char *vacuum_response);
void (*poll_vac_session)();

void vac_poll_idle();
void vac_poll_send_request();
void vac_poll_get_response();
void vac_response_timeout();
bool is_processed_short_vacuum_command(char *command);

#ifdef	__cplusplus
}
#endif

#endif	/* VACUUM_H */

