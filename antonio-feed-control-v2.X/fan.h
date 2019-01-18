/* 
 * File:   fan.h
 * Author: Jon's Windows
 *
 * Created on January 7, 2019, 10:12 AM
 */

#ifndef FAN_H
#define	FAN_H

#ifdef	__cplusplus
extern "C" {
#endif

#define FAN_MODE_AUTO   0
#define FAN_MODE_MANUAL 1

void set_fan_speed_callback();
void set_fan_speed_auto();
unsigned int compute_fan_pwm_from_reject_temp(float reject_temp_c);
unsigned int compute_fan_pwm_from_case_temp(float case_temp_c);
void set_fan_pwm(unsigned int percent);
void fan_sense_timeout_callback();
void poll_fan_sense();
void fan_sense_init();
void fan_sense_wait();
void fan_sense_read();
void setfanpwm_command(char *args[]);
void getfanpwm_command(char *args[]);
void getfanrpm_command(char *args[]);

unsigned int get_fan_mode();
void save_fan_state();
void load_fan_state();

void init_fan();

#ifdef MYFANAUTODEBUG
void test_fan_speed_auto()
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* FAN_H */

