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

#define AUTO_START_MAX_VAC_TIME_MIN 60
#define AUTO_START_MIN_VAC_TIME_MIN 10
#define AUTO_START_CRYO_HIGH_TEMP_K 285.0
#define AUTO_START_COLD_START_THRESHOLD_K 200.0
#define AUTO_START_SWITCH_TEMP_LIMIT_LOW_K 65.0
#define AUTO_START_SWITCH_TEMP_LIMIT_HIGH_K 300.0
#define AUTO_START_CRYO_SAFE_TEMP_C 60.0

#define MAX_FORE_VACUUM_TRIES 5
#define MAX_TURBO_POWER_TRIES 5
#define MAX_STANDBY_POWER_TRIES 20

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
void autostartgetbackingtime_command(char *args[]);
void autostartsetbackingtime_command(char *args[]);
void autostartgetstandbytime_command(char *args[]);
void autostartsetstandbytime_command(char *args[]);
void autostartgetturbotime_command(char *args[]);
void autostartsetturbotime_command(char *args[]);
//static void autostartgetvactime_command(char *args[], int * timevar);
//static void autostartsetvactime_command(char *args[], int * timevar,void (*savefun)(void));

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
void load_autostart_backing_time();
void save_autostart_backing_time();
void load_autostart_standby_time();
void save_autostart_standby_time();
void load_autostart_turbo_time();
void save_autostart_turbo_time();
//static void load_autostart_vacuum_time( int * val, char * filename );
//static void save_autostart_vacuum_time( int * val, char * filename );

void (*poll_auto_start)();

//two generic functions to send a request to vaccum
void autostart_generic_vacuum_request(char* vac_cmd, void (*next_fun)(void));
void autostart_generic_vacuum_response(char* vac_resp, void (*next_fun)(void), void (*err_fun)(void));
void autostart_timed_vacuum_response(char* vac_resp, void (*next_fun)(void), void (*err_fun)(void), int32_t delayticks);
void autostart_generic_cryo_request(char* cryo_cmd, void (*next_fun)(void));
void autostart_generic_cryo_response(float cryo_resp, void (*next_fun)(void), void (*err_fun)(void));

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
void auto_start_i013_request();
void auto_start_i013_response();
void auto_start_i014_request();
void auto_start_i014_response();

//autostart vacuum preparation
void auto_start_v001_request();
void auto_start_v001_response();
void auto_start_v002_request();
void auto_start_v002_response();
void auto_start_v004_request();
void auto_start_v004_response();
void auto_start_v005_request();
void auto_start_v005_response();
void auto_start_v005_fork_request();
void auto_start_v005_fork_response();
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
void auto_start_m001_request();
void auto_start_m001_response();
void auto_start_m002_request();
void auto_start_m002_response();
void auto_start_m003_request();
void auto_start_m003_response();

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
void auto_start_u001_response();
void auto_start_u002_request();

//error states
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
void auto_start_e010();
void auto_start_e011();

#ifdef	__cplusplus
}
#endif

#endif	/* AUTOSTART_H */

