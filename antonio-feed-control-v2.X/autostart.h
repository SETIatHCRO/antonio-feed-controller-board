/* 
 * File:   autostart.h
 * Author: Jon's Windows
 *
 * Created on January 7, 2019, 11:04 AM
 */

#ifndef AUTOSTART_H
#define	AUTOSTART_H

#ifdef	__cplusplus
extern "C" {
#endif

void (*auto_start_next_state)();
void (*auto_start_save_next_state)();

void auto_start_idle();
void auto_start_delay();

void auto_start_timer_callback();

void auto_start_000_p009_request();
void auto_start_000_p009_response();
void auto_start_001_p010_request();
void auto_start_001_p010_response();
void auto_start_002_p700_request();
void auto_start_002_p700_response();
void auto_start_003_p023_request();
void auto_start_003_p023_response();
void auto_start_004_p024_request();
void auto_start_004_p024_response();
void auto_start_005_p025_request();
void auto_start_005_p025_response();
void auto_start_006_p035_request();
void auto_start_006_p035_response();
void auto_start_007_p010_request();
void auto_start_007_p010_response();
void auto_start_008_getdiode();
void auto_start_008_sstop_request();
void auto_start_008_sstop_response();
void auto_start_009_p023_request();
void auto_start_009_p023_response();
void auto_start_010_p316_request();
void auto_start_010_p316_response();
void auto_start_011_p023_request();
void auto_start_011_p023_response();
void auto_start_012_sstop_request();
void auto_start_012_sstop_response();
void auto_start_013_getdiode();

void auto_start_999_sstop_request();
void auto_start_999_sstop_response();

void auto_start_send_request_to_vac();
void auto_start_send_request_to_vac_delayed();
void auto_start_get_response_from_vac();

void auto_start_send_request_to_cryo();
void auto_start_send_request_to_cryo_delayed();
void auto_start_get_response_from_cryo();

void auto_start_error();

void auto_start_complete();

float auto_start_getdiode();

bool get_auto_start_state();
void set_auto_start_state(bool state);
void load_autostart_state();

void save_autostart_state();

void (*poll_auto_start)();

#ifdef	__cplusplus
}
#endif

#endif	/* AUTOSTART_H */

