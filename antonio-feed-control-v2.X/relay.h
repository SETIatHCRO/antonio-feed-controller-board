/* 
 * File:   relay.h
 * Author: Jon's Windows
 *
 * Created on January 7, 2019, 11:32 AM
 */

#ifndef RELAY_H
#define	RELAY_H

#ifdef	__cplusplus
extern "C" {
#endif

void setrelay_command(char *args[]);
void getrelay_command(char *args[]);
void save_relay_state();
void load_relay_state();

#ifdef	__cplusplus
}
#endif

#endif	/* RELAY_H */

