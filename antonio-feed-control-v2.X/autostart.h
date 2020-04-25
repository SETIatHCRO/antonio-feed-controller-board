/* 
 * File:   autostart.h
 * Author: Jon's Windows
 *
 * Created on January 7, 2019, 11:04 AM
 */

#ifndef AUTOSTART_H
#define	AUTOSTART_H

#include <stdint.h>         /* For uint32_t definition                        */
#include <stdbool.h>        /* For true/false definition     */


#ifdef	__cplusplus
extern "C" {
#endif

//shared between autostart.c and various autostart_x.c
#define AUTO_START_1_MIN 60*1000
#define AUTO_START_5_MIN 5*60*1000
#define AUTO_START_10_MIN 10*60*1000
#define AUTO_START_15_MIN 15*60*1000
#define AUTO_START_30_MIN 30*60*1000
#define AUTO_START_45_MIN 45*60*1000

#define AUTO_START_CMND_RSPNS_MAX_LEN 99
#define AUTO_START_CMND_RSPNS_MAX_TRIES 2

void (*auto_start_next_state)();
void (*auto_start_save_next_state)();

void auto_start_idle();
void auto_start_delay();

void auto_start_timer_callback();

int autostart_cryo_getfloatfromresp(float * val);
int autostart_vac_getulongfromresp(unsigned long int * val);

void autostart_command(char *args[]);
void shutdown_command(char *args[]);
void getswitchtemp_command(char *args[]);
void setswitchtemp_command(char *args[]);
void autostartgetstate_command(char *args[]);

void auto_start_send_request_to_vac();
void auto_start_send_request_to_vac_delayed();
void auto_start_get_response_from_vac();

void auto_start_check_vacuum_request();
void auto_start_check_vacuum_response();

void auto_start_send_request_to_cryo();
void auto_start_send_request_to_cryo_delayed();
void auto_start_get_response_from_cryo();

void auto_start_error();

void auto_start_complete();
void shutdown_complete();

float auto_start_getdiode();

bool get_auto_start_state();
void set_auto_start_state(bool state);

void load_autostart_state();
void save_autostart_state();
void load_autostart_switchTemp();
void save_autostart_switchTemp();

void (*poll_auto_start)();

//two generic functions to send a request to vaccum
void autostart_generic_vacuum_request(char* vac_cmd, void (*next_fun)(void));
void autostart_generic_vacuum_response(char* vac_resp, void (*next_fun)(void), void (*err_fun)(void));
void autostart_timed_vacuum_response(char* vac_resp, void (*next_fun)(void), void (*err_fun)(void), int32_t delayticks);
void autostart_generic_cryo_request(char* cryo_cmd, void (*next_fun)(void));
void autostart_generic_cryo_response(float cryo_resp, void (*next_fun)(void), void (*err_fun)(void));

void autostart_set_ttarget_withdelta_request(float target_t,  void (*next_fun)(void));
void autostart_set_ttarget_withdelta_response(void (*next_fun)(void), void (*err_fun)(void));

void autostart_test_vacuum_param_true(void (*true_fun)(void), void (*false_fun)(void), void (*err_fun)(void));

//Init sequence of auto start
void auto_start_i000_request();
void auto_start_i000_response();
void auto_start_i001_request();
void auto_start_i002a_request();
void auto_start_i002a_response();
void auto_start_i002b_request();
void auto_start_i002b_response();
void auto_start_i003a_request();
void auto_start_i003a_response();
void auto_start_i004a_request();
void auto_start_i004a_response();
void auto_start_i005a_request();
void auto_start_i005a_response();
void auto_start_i006a_request();
void auto_start_i006a_response();
void auto_start_i003b_request();
void auto_start_i003b_response();
void auto_start_i004b_request();
void auto_start_i004b_response();
void auto_start_i005b_request();
void auto_start_i005b_response();
void auto_start_i006b_request();
void auto_start_i006b_response();
void auto_start_i007_request();
void auto_start_i007_response();
void auto_start_i008_request();
void auto_start_i008_response();
void auto_start_i009_request();
void auto_start_i009_response();
void auto_start_i010_request();
void auto_start_i010_response();
void auto_start_i011_request();
void auto_start_i011_response();
void auto_start_i012_request();
void auto_start_i012_response();

//autostart vacuum preparation
void auto_start_v001_request();
void auto_start_v001_response();
void auto_start_v002_request();
void auto_start_v002_response();
void auto_start_v004_request();
void auto_start_v004_response();
void auto_start_v005_request();
void auto_start_v005_response();
void auto_start_v006_request();
void auto_start_v006_response();
void auto_start_v007_request();
void auto_start_v007_response();
void auto_start_v008_request();
void auto_start_v008_response();
void auto_start_v009_request();
void auto_start_v009_response();
void auto_start_v010_request();
void auto_start_v010_response();
void auto_start_v011_request();
void auto_start_v011_response();

//shutdown end steps
void auto_start_s001_request();
void auto_start_s001_response();
void auto_start_s002_request();
void auto_start_s002_response();
void auto_start_s003_request();
void auto_start_s003_response();

//stepping temperature down
void auto_start_d001_request();
void auto_start_d001_response();
void auto_start_d002_request();
void auto_start_d002_response();
void auto_start_d003_request();
void auto_start_d003_response();
void auto_start_d004_request();
void auto_start_d004_response();
void auto_start_d005_request();
void auto_start_d005_response();
void auto_start_d006_request();
void auto_start_d006_response();
void auto_start_d007_request();
void auto_start_d007_response();
void auto_start_d008_request();
void auto_start_d008_response();
void auto_start_d009_request();
void auto_start_d009_response();
void auto_start_d010_request();
void auto_start_d010_response();
void auto_start_d011_request();

//stepping temperature up
void auto_start_u001_request();

void auto_start_e000();
void auto_start_e001();
void auto_start_e002();
void auto_start_e003();
void auto_start_e004();
void auto_start_e005();
void auto_start_e006();
void auto_start_e007();
void auto_start_e008();
void auto_start_e009();

/*void auto_start_000_p009_request();
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
*/

#ifdef	__cplusplus
}
#endif

#endif	/* AUTOSTART_H */

