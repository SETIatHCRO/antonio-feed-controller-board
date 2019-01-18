#include <plib.h>           /* Include to use PIC32 peripheral libraries      */

#include <stdio.h>
#include <string.h>

#include "system.h"

#include "tc74.h"
#include "fan.h"
#include "oneshot.h"
#include "fatfs/diskio.h"
#include "fatfs/ff.h"

extern char OK[];
extern char *EOL;

unsigned int fan_mode = FAN_MODE_MANUAL;

const unsigned int min_fan_pwm = 20;
const unsigned int cold_fan_pwm = 30;
const unsigned int warm_fan_pwm = 66;
const unsigned int default_fan_pwm = 90;
const unsigned int hot_fan_pwm = 100;

const float max_reject_temp = 70.0;
const float hot_reject_temp = 55.0;
const float warm_reject_temp = 40.0;
const float cold_reject_temp = 0.0;

const float max_case_temp = 80.0;
const float hot_case_temp = 65.0;
const float warm_case_temp = 50.0;
const float cold_case_temp = 0.0;

unsigned int fan_pwm_percent = 90;

struct oneshot_timer fan_speed_timer = {"fan_speed_timer", 0, NULL, NULL};
struct oneshot_timer fan_sense_timeout_timer = {"fan_sense_timeout", 0, NULL, NULL};

uint16_t fan_sense_previous_captured_value = 0;

float fan_pulses_per_rotation = 2;

unsigned int fan_rpm = 0;

void (*fan_sense_state)();

unsigned int auto_fan_reject_tc74_i = 5;
unsigned int auto_fan_case_tc74_i = 6;

void init_fan() {
    start_timer(&fan_speed_timer, set_fan_speed_callback, 10000);

    fan_sense_state = fan_sense_init;
}

void set_fan_speed_callback() {
#ifdef MYFANAUTODEBUG
    char msg[49];
#endif
    if (fan_mode == FAN_MODE_AUTO) {
        set_fan_speed_auto();
    }

#ifdef MYFANAUTODEBUG
    sprintf(msg, "fan_pwm_percent is %u", fan_pwm_percent);
    send_to_rimbox(msg);
    send_to_rimbox(EOL);
#endif

    set_fan_pwm(fan_pwm_percent);

    start_timer(&fan_speed_timer, set_fan_speed_callback, 10000);
}

#ifdef MYFANAUTODEBUG
void test_fan_speed_auto() {
    char msg[99];

    float temp;

    for (temp = -10.0; temp < 80.0; temp += 1.0) {
        sprintf(msg, "%5.1f  %3u  %3u", temp,
                compute_fan_pwm_from_reject_temp(temp),
                compute_fan_pwm_from_case_temp(temp));
        mylog(msg);
    }
}
#endif

void set_fan_speed_auto() {
#ifdef MYFANAUTODEBUG
    char msg[49];
#endif

    unsigned int auto_reject_pwm;
    unsigned int auto_case_pwm;

    unsigned int auto_fan_pwm = default_fan_pwm;

    struct tc74 *tc74_devices;

    gettc74table(&tc74_devices);

    if (tc74_devices[auto_fan_reject_tc74_i].is_initialized &&
              tc74_devices[auto_fan_reject_tc74_i].is_valid_temp) {
        auto_reject_pwm = compute_fan_pwm_from_reject_temp(
                tc74_devices[auto_fan_reject_tc74_i].temp_c);
        auto_fan_pwm = auto_reject_pwm;
    }

    if (tc74_devices[auto_fan_case_tc74_i].is_initialized &&
              tc74_devices[auto_fan_case_tc74_i].is_valid_temp) {
        auto_case_pwm = compute_fan_pwm_from_case_temp(
                tc74_devices[auto_fan_case_tc74_i].temp_c);
        if (auto_case_pwm > auto_fan_pwm) {
            auto_fan_pwm = auto_case_pwm;
        }
    }

    fan_pwm_percent = auto_fan_pwm;
}

unsigned int compute_fan_pwm_from_reject_temp(float reject_temp_c) {
    unsigned int fan_pwm = default_fan_pwm;

    if (reject_temp_c > hot_reject_temp) {
        fan_pwm = hot_fan_pwm;
        return (fan_pwm);
    }

    if (reject_temp_c > warm_reject_temp) {
        fan_pwm = warm_fan_pwm + (unsigned int)
            (((reject_temp_c - warm_reject_temp) /
                (hot_reject_temp - warm_reject_temp)) *
            ((float) (hot_fan_pwm - warm_fan_pwm)));
        return (fan_pwm);
    }

    if (reject_temp_c > cold_reject_temp) {
        fan_pwm = cold_fan_pwm + (unsigned int)
            (((reject_temp_c - cold_reject_temp) /
                (warm_reject_temp - cold_reject_temp)) *
            ((float) (warm_fan_pwm - cold_fan_pwm)));
        return (fan_pwm);
    }

    fan_pwm = cold_fan_pwm;

    return (fan_pwm);
}

unsigned int compute_fan_pwm_from_case_temp(float case_temp_c) {
    unsigned int fan_pwm = default_fan_pwm;

    if (case_temp_c > hot_case_temp) {
        fan_pwm = hot_fan_pwm;
        return (fan_pwm);
    }

    if (case_temp_c > warm_case_temp) {
        fan_pwm = warm_fan_pwm + (unsigned int)
            (((case_temp_c - warm_case_temp) /
                (hot_case_temp - warm_case_temp)) *
            ((float) (hot_fan_pwm - warm_fan_pwm)));
        return (fan_pwm);
    }

    if (case_temp_c > cold_case_temp) {
        fan_pwm = cold_fan_pwm + (unsigned int)
            (((case_temp_c - cold_case_temp) /
                (warm_case_temp - cold_case_temp)) *
            ((float) (warm_fan_pwm - cold_fan_pwm)));
        return (fan_pwm);
    }

    fan_pwm = cold_fan_pwm;

    return (fan_pwm);
}

void set_fan_pwm(unsigned int percent) {
    if (percent > 99) {
        percent = 99;
    }
    if (percent < min_fan_pwm) {
        percent = min_fan_pwm;
    }

    unsigned int inverse_duty_cycle = 800 - ((800 * percent) / 100);

    SetPulseOC2(inverse_duty_cycle, 800);
}

void fan_sense_timeout_callback() {
    fan_sense_state = fan_sense_init;

    fan_rpm = 0;
    
#ifdef MYFANSENSEDEBUG
    send_to_rimbox("fan sense timeout");
    send_to_rimbox(EOL);
#endif
}

void poll_fan_sense() {
    fan_sense_state();
}

void fan_sense_init() {
    fan_sense_state = fan_sense_wait;

    start_timer(&fan_sense_timeout_timer, fan_sense_timeout_callback, 3000);
}

void fan_sense_wait() {
    if (mIC1CaptureReady()) {
        fan_sense_state = fan_sense_read;
    }
}

/*
 * todo: eliminate hard-coded 800 value!  Set output pin low or
 *       high for 0 and 100 percent.  Also, see errata item 9.
 */
void fan_sense_read() {
    start_timer(&fan_sense_timeout_timer, fan_sense_timeout_callback, 3000);

#ifdef MYFANSENSEDEBUG
    char msg[19];
#endif
    uint16_t captured_value;
    uint16_t elapsed_value;

    captured_value = mIC1ReadCapture();
    elapsed_value = (uint16_t) (captured_value - fan_sense_previous_captured_value);
    fan_rpm = (unsigned int) (60.0 * ((PB_FREQ / 256.0) /
            ((fan_pulses_per_rotation * elapsed_value) / 4.0)));
#ifdef MYFANSENSEDEBUG
    sprintf(msg, "%05u  %04u", elapsed_value, fan_rpm);
    mylog(msg);
#endif
    fan_sense_previous_captured_value = captured_value;

    fan_sense_state = fan_sense_wait;
}

void setfanpwm_command(char *args[]) {
    char msg[99];
    unsigned int i;
    int N;
    unsigned int fan_pwm_arg;

    if (args[0] == NULL) {
        send_to_rimbox(EOL);
        return;
    }

    if (strcasecmp(args[0], "auto") == 0) {
        fan_mode = FAN_MODE_AUTO;
        save_fan_state();
        send_to_rimbox(OK);
        send_to_rimbox(EOL);
        return;
    }

    N = sscanf(args[0], "%u", &fan_pwm_arg);

    if (N != 1) {
        send_to_rimbox(EOL);
        return;
    }

    if (!((fan_pwm_arg >= 0) && (fan_pwm_arg <= 100))) {
        send_to_rimbox(EOL);
        return;
    }

    fan_pwm_percent = fan_pwm_arg;

    fan_mode = FAN_MODE_MANUAL;
    if (fan_pwm_percent < min_fan_pwm) {
        fan_pwm_percent = min_fan_pwm;
    }
    if (fan_pwm_percent > 100) {
        fan_pwm_percent = 100;
    }

    save_fan_state();

    set_fan_pwm(fan_pwm_percent);

    send_to_rimbox(OK);
    send_to_rimbox(EOL);
}

void getfanpwm_command(char *args[]) {
    char msg[19];

    if (fan_mode == FAN_MODE_AUTO) {
        strcpy(msg, "auto");
    }
    else {
        sprintf(msg, "%u", fan_pwm_percent);
    }

    send_to_rimbox(msg);
    send_to_rimbox(EOL);
}

void getfanrpm_command(char *args[]) {
    char msg[19];

    sprintf(msg, "%u", fan_rpm);
    send_to_rimbox(msg);
    send_to_rimbox(EOL);
}

unsigned int get_fan_mode() {
    return fan_mode;
}

void save_fan_state() {
    FIL fp;
    FRESULT rslt;
    UINT bytes_written;

    char text_state[19];

    if (fan_mode == FAN_MODE_AUTO) {
        strcpy(text_state, "auto");
    }
    else {
        sprintf(text_state, "%u", fan_pwm_percent);
    }

    rslt = f_open(&fp, "FANSTATE.TXT", (FA_CREATE_ALWAYS | FA_WRITE));
    if (rslt == FR_OK) {
        f_write(&fp, text_state, strlen(text_state), &bytes_written);
        f_close(&fp);
    }
}

/*
 * todo: remove redundancy between code in this function and code
 * in function setfanpwm_command
 */
void load_fan_state() {
    FIL fp;
    FRESULT rslt;
    UINT bytes_read;
    int N;
    unsigned int init_fan_pwm;

    char text_state[19];

    fan_mode = FAN_MODE_AUTO;
    fan_pwm_percent = default_fan_pwm;
    set_fan_pwm(fan_pwm_percent); 

    rslt = f_open(&fp, "FANSTATE.TXT", (FA_READ));
    if (rslt != FR_OK) {
        return;
    }

    f_read(&fp, text_state, 4, &bytes_read);
    f_close(&fp);
    
    if (!(bytes_read >= 1)) {
        return;
    }

    text_state[bytes_read] = 0;
    if (strcasecmp(text_state, "auto") == 0) {
       fan_mode = FAN_MODE_AUTO;
       return;
    }

    N = sscanf(text_state, "%u", &init_fan_pwm);

    if (N != 1) {
        return;
    }

    if (!((init_fan_pwm >= 0) && (init_fan_pwm <= 100))) {
        return;
    }

    fan_pwm_percent = init_fan_pwm;

    fan_mode = FAN_MODE_MANUAL;
    if (fan_pwm_percent < min_fan_pwm) {
        fan_pwm_percent = min_fan_pwm;
    }
    if (fan_pwm_percent > 100) {
        fan_pwm_percent = 100;
    }

    set_fan_pwm(fan_pwm_percent); 
}