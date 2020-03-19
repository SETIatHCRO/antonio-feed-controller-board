#include <stdint.h>         /* For uint32_t definition                        */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>        /* For true/false definition     */

#include "autostart.h"
#include "oneshot.h"
#include "diode.h"
#include "rimbox.h"
#include "vacuum.h"
#include "cryo.h"
#include "fatfs/diskio.h"
#include "fatfs/ff.h"
#include "file_utils.h"
#include "adc.h"

extern bool is_parse_vacuum_response;

extern bool is_vac_response_ready;
extern bool is_vac_busy;

#define MAX_COMMAND_LEN 999
#define MAX_RESPONSE_LEN 999

extern bool is_cryo_busy;
extern bool is_cryo_response_ready;

extern char vac_request[99];
extern char vac_response[99];
extern char cryo_response[99];

struct oneshot_timer auto_start_timer = {"auto_start_timer", 0, NULL, NULL};


#define AUTO_START_GETDIODE_THRESHOLD 250.0

#define AUTO_START_MAX_TRIES 3



unsigned int auto_start_tries = 0;

unsigned int auto_start_cmnd_rspns_tries = 0;

char auto_start_request[AUTO_START_CMND_RSPNS_MAX_LEN];

char auto_start_response[AUTO_START_CMND_RSPNS_MAX_LEN];

bool auto_start_state = false;

bool should_report_complete = true;

bool doing_startup = false;
bool doing_shutdown = false;

float lastTemp = 350.0;
float targetTemp = 85.0;
float highTemp = 273;

int fore_vacuum_try = 0;
int turbo_power_try = 0;

// TODO: embed scripting engine for auto start procedure

void auto_start_idle() {
    if (auto_start_state == true) {
        doing_startup = true;
        start_timer(&auto_start_timer, auto_start_timer_callback,
                AUTO_START_1_MIN);
        auto_start_next_state = auto_start_i000_request;
        poll_auto_start = auto_start_delay;
        send_to_rimbox("\r\nautostart in 1 minute\r\n");
    }
}

void auto_start_delay() {
    
}

void auto_start_timer_callback() {
    
    poll_auto_start = auto_start_next_state;
}

void autostart_generic_vacuum_request(char* vac_cmd, void (*next_fun)(void)) {
    //strcpy(auto_start_request, vac_cmd);
    strncpy(auto_start_request, vac_cmd, AUTO_START_CMND_RSPNS_MAX_LEN-1);

    auto_start_cmnd_rspns_tries = 0;

    auto_start_next_state = next_fun;
    poll_auto_start = auto_start_send_request_to_vac;
}

void autostart_generic_vacuum_response(char* vac_resp, void (*next_fun)(void), void (*err_fun)(void)) {
    if (strcmp(auto_start_response, vac_resp) == 0) {
        poll_auto_start = next_fun;
        return;
    }

    auto_start_cmnd_rspns_tries += 1;

    if (auto_start_cmnd_rspns_tries < AUTO_START_CMND_RSPNS_MAX_TRIES) {
        poll_auto_start = auto_start_send_request_to_vac;
        return;
    }

    poll_auto_start = err_fun;
}

void autostart_timed_vacuum_response(char* vac_resp, void (*next_fun)(void), void (*err_fun)(void), int32_t delayticks) {
    if (strcmp(auto_start_response, vac_resp) == 0) {
        feedlog("waiting...");
        start_timer(&auto_start_timer, auto_start_timer_callback,delayticks);
        auto_start_next_state = next_fun;
        poll_auto_start = auto_start_delay;
        return;
    }

    auto_start_cmnd_rspns_tries += 1;

    if (auto_start_cmnd_rspns_tries < AUTO_START_CMND_RSPNS_MAX_TRIES) {
        poll_auto_start = auto_start_send_request_to_vac;
        return;
    }

    poll_auto_start = err_fun;
}

void autostart_generic_cryo_request(char* cryo_cmd, void (*next_fun)(void)) {
    //strcpy(auto_start_request, cryo_cmd);
    strncpy(auto_start_request, cryo_cmd, AUTO_START_CMND_RSPNS_MAX_LEN-1);

    auto_start_cmnd_rspns_tries = 0;

    auto_start_next_state = next_fun;
    poll_auto_start = auto_start_send_request_to_cryo;
}

void autostart_generic_cryo_response(float cryo_resp, void (*next_fun)(void), void (*err_fun)(void)) {
    float cryo_flt_rspns;

    int N = sscanf(auto_start_response, "%f", &cryo_flt_rspns);

    if ((N == 1) && (cryo_flt_rspns == cryo_resp)) {
        poll_auto_start = next_fun;
        return;
    }

    auto_start_cmnd_rspns_tries += 1;

    if (auto_start_cmnd_rspns_tries < AUTO_START_CMND_RSPNS_MAX_TRIES) {
        poll_auto_start = auto_start_send_request_to_cryo;
        return;
    }

    poll_auto_start = err_fun;
}

//This is commented out "old" part of the autostart routine
/*void auto_start_000_p009_request() {
    
    send_to_rimbox("autostart starting\r\n");
    
    should_report_complete = true;
    
    strcpy(auto_start_request, "p009=111111");

    auto_start_cmnd_rspns_tries = 0;

    auto_start_next_state = auto_start_000_p009_response;
    poll_auto_start = auto_start_send_request_to_vac;
}

void auto_start_000_p009_response() {
    if (strcmp(auto_start_response, "111111") == 0) {
        poll_auto_start = auto_start_001_p010_request;
        return;
    }

    auto_start_cmnd_rspns_tries += 1;

    if (auto_start_cmnd_rspns_tries < AUTO_START_CMND_RSPNS_MAX_TRIES) {
        poll_auto_start = auto_start_send_request_to_vac;
        return;
    }

    poll_auto_start = auto_start_error;
}

void auto_start_001_p010_request() {
    strcpy(auto_start_request, "p010=000000");

    auto_start_cmnd_rspns_tries = 0;

    auto_start_next_state = auto_start_001_p010_response;
    poll_auto_start = auto_start_send_request_to_vac;
}

void auto_start_001_p010_response() {
    if (strcmp(auto_start_response, "000000") == 0) {
        poll_auto_start = auto_start_002_p700_request;
        return;
    }

    auto_start_cmnd_rspns_tries += 1;

    if (auto_start_cmnd_rspns_tries < AUTO_START_CMND_RSPNS_MAX_TRIES) {
        poll_auto_start = auto_start_send_request_to_vac;
        return;
    }

    poll_auto_start = auto_start_error;
}

void auto_start_002_p700_request() {
    strcpy(auto_start_request, "p700=000020");

    auto_start_cmnd_rspns_tries = 0;

    auto_start_next_state = auto_start_002_p700_response;
    poll_auto_start = auto_start_send_request_to_vac;
}

void auto_start_002_p700_response() {
    if (strcmp(auto_start_response, "000020") == 0) {
        poll_auto_start = auto_start_003_p023_request;
        return;
    }

    auto_start_cmnd_rspns_tries += 1;

    if (auto_start_cmnd_rspns_tries < AUTO_START_CMND_RSPNS_MAX_TRIES) {
        poll_auto_start = auto_start_send_request_to_vac;
        return;
    }

    poll_auto_start = auto_start_error;
}

void auto_start_003_p023_request() {
    strcpy(auto_start_request, "p023=111111");

    auto_start_cmnd_rspns_tries = 0;

    auto_start_next_state = auto_start_003_p023_response;
    poll_auto_start = auto_start_send_request_to_vac;
}

void auto_start_003_p023_response() {
    if (strcmp(auto_start_response, "111111") == 0) {
        poll_auto_start = auto_start_004_p024_request;
        return;
    }

    auto_start_cmnd_rspns_tries += 1;

    if (auto_start_cmnd_rspns_tries < AUTO_START_CMND_RSPNS_MAX_TRIES) {
        poll_auto_start = auto_start_send_request_to_vac;
        return;
    }

    poll_auto_start = auto_start_error;
}

void auto_start_004_p024_request() {
    strcpy(auto_start_request, "p024=000");

    auto_start_cmnd_rspns_tries = 0;

    auto_start_next_state = auto_start_004_p024_response;
    poll_auto_start = auto_start_send_request_to_vac;
}

void auto_start_004_p024_response() {
    if (strcmp(auto_start_response, "000") == 0) {
        poll_auto_start = auto_start_005_p025_request;
        return;
    }

    auto_start_cmnd_rspns_tries += 1;

    if (auto_start_cmnd_rspns_tries < AUTO_START_CMND_RSPNS_MAX_TRIES) {
        poll_auto_start = auto_start_send_request_to_vac;
        return;
    }

    poll_auto_start = auto_start_error;
}

void auto_start_005_p025_request() {
    strcpy(auto_start_request, "p025=000");

    auto_start_cmnd_rspns_tries = 0;

    auto_start_next_state = auto_start_005_p025_response;
    poll_auto_start = auto_start_send_request_to_vac;
}

void auto_start_005_p025_response() {
    if (strcmp(auto_start_response, "000") == 0) {
        poll_auto_start = auto_start_006_p035_request;
        return;
    }

    auto_start_cmnd_rspns_tries += 1;

    if (auto_start_cmnd_rspns_tries < AUTO_START_CMND_RSPNS_MAX_TRIES) {
        poll_auto_start = auto_start_send_request_to_vac;
        return;
    }

    poll_auto_start = auto_start_error;
}

void auto_start_006_p035_request() {
    strcpy(auto_start_request, "p035=003");

    auto_start_cmnd_rspns_tries = 0;

    auto_start_next_state = auto_start_006_p035_response;
    poll_auto_start = auto_start_send_request_to_vac;
}

void auto_start_006_p035_response() {
    if (strcmp(auto_start_response, "003") == 0) {
        poll_auto_start = auto_start_007_p010_request;
        return;
    }

    auto_start_cmnd_rspns_tries += 1;

    if (auto_start_cmnd_rspns_tries < AUTO_START_CMND_RSPNS_MAX_TRIES) {
        poll_auto_start = auto_start_send_request_to_vac;
        return;
    }

    poll_auto_start = auto_start_error;
}

void auto_start_007_p010_request() {
    strcpy(auto_start_request, "p010=111111");

    auto_start_cmnd_rspns_tries = 0;

    auto_start_next_state = auto_start_007_p010_response;
    poll_auto_start = auto_start_send_request_to_vac;
}

void auto_start_007_p010_response() {
    if (strcmp(auto_start_response, "111111") == 0) {
        poll_auto_start = auto_start_008_getdiode;
        return;
    }

    auto_start_cmnd_rspns_tries += 1;

    if (auto_start_cmnd_rspns_tries < AUTO_START_CMND_RSPNS_MAX_TRIES) {
        poll_auto_start = auto_start_send_request_to_vac;
        return;
    }

    poll_auto_start = auto_start_error;
}

void auto_start_008_getdiode() {
    float tempk;

    tempk = auto_start_getdiode();

    if (poll_auto_start == auto_start_error) {
        return;
    }

    if (tempk < AUTO_START_GETDIODE_THRESHOLD) {
        poll_auto_start = auto_start_008_sstop_request;
        return;
    }

    poll_auto_start = auto_start_009_p023_request;
}

void auto_start_008_sstop_request() {
    strcpy(auto_start_request, "SET SSTOP=0");

    auto_start_cmnd_rspns_tries = 0;

    auto_start_next_state = auto_start_008_sstop_response;
    poll_auto_start = auto_start_send_request_to_cryo;
}

void auto_start_008_sstop_response() {
    float cryo_flt_rspns;

    int N = sscanf(auto_start_response, "%f", &cryo_flt_rspns);

    if ((N == 1) && (cryo_flt_rspns == 0.0)) {
        poll_auto_start = auto_start_009_p023_request;
        return;
    }

    auto_start_cmnd_rspns_tries += 1;

    if (auto_start_cmnd_rspns_tries < AUTO_START_CMND_RSPNS_MAX_TRIES) {
        poll_auto_start = auto_start_send_request_to_cryo;
        return;
    }

    poll_auto_start = auto_start_error;
}

void auto_start_009_p023_request() {
    strcpy(auto_start_request, "p023=111111");

    auto_start_cmnd_rspns_tries = 0;

    auto_start_next_state = auto_start_009_p023_response;
    poll_auto_start = auto_start_send_request_to_vac;
}

void auto_start_009_p023_response() {
    if (strcmp(auto_start_response, "111111") == 0) {
        feedlog("waiting 15 minutes...");
        start_timer(&auto_start_timer, auto_start_timer_callback,
                AUTO_START_15_MIN);
        auto_start_next_state = auto_start_010_p316_request;
        poll_auto_start = auto_start_delay;
        return;
    }

    auto_start_cmnd_rspns_tries += 1;

    if (auto_start_cmnd_rspns_tries < AUTO_START_CMND_RSPNS_MAX_TRIES) {
        poll_auto_start = auto_start_send_request_to_vac;
        return;
    }

    poll_auto_start = auto_start_error;
}

void auto_start_010_p316_request() {
    strcpy(auto_start_request, "p316");

    auto_start_cmnd_rspns_tries = 0;

    auto_start_next_state = auto_start_010_p316_response;
    poll_auto_start = auto_start_send_request_to_vac;
}

void auto_start_010_p316_response() {
    unsigned int p316_val;

    int N = sscanf(auto_start_response, "%u", &p316_val);

    if (N == 1) {
        if (p316_val <= 20) {
            poll_auto_start = auto_start_999_sstop_request;
        }
        else {
            poll_auto_start = auto_start_011_p023_request;
        }
        return;
    }

    auto_start_cmnd_rspns_tries += 1;

    if (auto_start_cmnd_rspns_tries < AUTO_START_CMND_RSPNS_MAX_TRIES) {
        poll_auto_start = auto_start_send_request_to_vac;
        return;
    }

    poll_auto_start = auto_start_error;
}

void auto_start_011_p023_request() {
    strcpy(auto_start_request, "p023=000000");

    auto_start_cmnd_rspns_tries = 0;

    auto_start_next_state = auto_start_011_p023_response;
    poll_auto_start = auto_start_send_request_to_vac;
}

void auto_start_011_p023_response() {
    if (strcmp(auto_start_response, "000000") == 0) {
        poll_auto_start = auto_start_012_sstop_request;
        return;
    }

    auto_start_cmnd_rspns_tries += 1;

    if (auto_start_cmnd_rspns_tries < AUTO_START_CMND_RSPNS_MAX_TRIES) {
        poll_auto_start = auto_start_send_request_to_vac;
        return;
    }

    poll_auto_start = auto_start_error;
}

void auto_start_012_sstop_request() {
    strcpy(auto_start_request, "SET SSTOP=1");

    auto_start_cmnd_rspns_tries = 0;

    auto_start_next_state = auto_start_012_sstop_response;
    poll_auto_start = auto_start_send_request_to_cryo;
}

void auto_start_012_sstop_response() {
    float cryo_flt_rspns;

    int N = sscanf(auto_start_response, "%f", &cryo_flt_rspns);

    if ((N == 1) && (cryo_flt_rspns == 1.0)) {
        poll_auto_start = auto_start_013_getdiode;
        return;
    }

    auto_start_cmnd_rspns_tries += 1;

    if (auto_start_cmnd_rspns_tries < AUTO_START_CMND_RSPNS_MAX_TRIES) {
        poll_auto_start = auto_start_send_request_to_cryo;
        return;
    }

    poll_auto_start = auto_start_error;
}

void auto_start_013_getdiode() {
    float tempk;

    tempk = auto_start_getdiode();

    if (poll_auto_start == auto_start_error) {
        return;
    }

    if (tempk < AUTO_START_GETDIODE_THRESHOLD) {
        poll_auto_start = auto_start_error;
        return;
    }

    auto_start_tries += 1;
    if (auto_start_tries < AUTO_START_MAX_TRIES) {
        feedlog("waiting 45 minutes...");
        start_timer(&auto_start_timer, auto_start_timer_callback,
                AUTO_START_45_MIN);
        auto_start_next_state = auto_start_009_p023_request;
        poll_auto_start = auto_start_delay;
        return;
    }

    poll_auto_start = auto_start_error;
}

void auto_start_999_sstop_request() {
    strcpy(auto_start_request, "SET SSTOP=0");

    auto_start_cmnd_rspns_tries = 0;

    auto_start_next_state = auto_start_999_sstop_response;
    poll_auto_start = auto_start_send_request_to_cryo;
}

void auto_start_999_sstop_response() {
    float cryo_flt_rspns;

    int N = sscanf(auto_start_response, "%f", &cryo_flt_rspns);

    if ((N == 1) && (cryo_flt_rspns == 0.0)) {
        poll_auto_start = auto_start_complete;
        return;
    }

    auto_start_cmnd_rspns_tries += 1;

    if (auto_start_cmnd_rspns_tries < AUTO_START_CMND_RSPNS_MAX_TRIES) {
        poll_auto_start = auto_start_send_request_to_cryo;
        return;
    }

    poll_auto_start = auto_start_error;
}
*/
void auto_start_send_request_to_vac() {
    auto_start_save_next_state = auto_start_next_state;
    auto_start_next_state = auto_start_send_request_to_vac_delayed;

    start_timer(&auto_start_timer, auto_start_timer_callback, 3000);
    poll_auto_start = auto_start_delay;
}

void auto_start_send_request_to_vac_delayed() {
    if (is_vac_busy) {
        return;
    }

    auto_start_next_state = auto_start_save_next_state;

    feedlog(auto_start_request);    

    send_command_to_vacuum(auto_start_request);
    is_processed_short_vacuum_command(vac_request);

    poll_auto_start = auto_start_get_response_from_vac;
}

void auto_start_get_response_from_vac() {
    if (!(is_vac_response_ready)) {
        return;
    }

    parse_vacuum_response(vac_response);
    is_parse_vacuum_response = false;

    strcpy(auto_start_response, vac_response);

    feedlog(auto_start_response);

    free_vacuum_session();

    poll_auto_start = auto_start_next_state;
}

void auto_start_send_request_to_cryo() {
    auto_start_save_next_state = auto_start_next_state;
    auto_start_next_state = auto_start_send_request_to_cryo_delayed;

    start_timer(&auto_start_timer, auto_start_timer_callback, 3000);
    poll_auto_start = auto_start_delay;
}

void auto_start_send_request_to_cryo_delayed() {
    if (is_cryo_busy) {
        return;
    }

    auto_start_next_state = auto_start_save_next_state;

    feedlog(auto_start_request);

    send_command_to_cryo(auto_start_request, NULL);

    poll_auto_start = auto_start_get_response_from_cryo;
}

void auto_start_get_response_from_cryo() {
    char *rspnsln;

    if (!(is_cryo_response_ready)) {
        return;
    }

    rspnsln = strtok(cryo_response, "\r");
    if (rspnsln == NULL) {
        strcpy (auto_start_response, "");
    }
    else {
        strcpy (auto_start_response, rspnsln);
    }

    feedlog(auto_start_response);

    free_cryo_session();

    poll_auto_start = auto_start_next_state;
}

void auto_start_error() {
    
    if(should_report_complete) {
        send_to_rimbox("\r\nautostart error\r\n");
        send_to_rimbox("hanging\r\n");
        should_report_complete = false;
        doing_startup = false;
    }
    doing_shutdown = true;
}

void auto_start_complete() {
   
    doing_startup = false;
    if(should_report_complete) {
        send_to_rimbox("\r\nautostart complete\r\n");
        should_report_complete = false;
    }
    if(doing_shutdown) {
        poll_auto_start = auto_start_u001_request;
        should_report_complete = true;
    }
   
}

void shutdown_complete() {
    doing_shutdown = false;
    if(should_report_complete) {
        send_to_rimbox("\r\nshutdown complete\r\n");
        should_report_complete = false;
    }
    if (doing_startup) {
        poll_auto_start = auto_start_i000_request;
        should_report_complete = true;
    }
}

void autostart_command() {
    send_to_rimbox("\rntoggling to auto start\r\n");
    doing_startup = true;
    doing_shutdown = false;
    //if we were waiting for a timer - making sure timer ends in 3s
    //it's not the most graceful part of the code though
    if(auto_start_timer.countdown_ticks > 3000)
        auto_start_timer.countdown_ticks = 3000;
}

void shutdown_command() {
    send_to_rimbox("\rntoggling to shutdown\r\n");
    doing_startup = false;
    doing_shutdown = true;
    //if we were waiting for a timer - making sure timer ends in 3s
    //it's not the most graceful part of the code though
    if(auto_start_timer.countdown_ticks > 3000)
        auto_start_timer.countdown_ticks = 3000;
}

float auto_start_getdiode() {
    char msg[19];
    unsigned int i;

    float adc3_value;
    float diode_voltage;
    float tempk;
    float m;
    float b;

    struct adc *adc_inputs;

    unsigned int nmbr_of_adc_inputs = getadctable(&adc_inputs);

    feedlog("getdiode");

    // it is assumed there is an address 3 in the table
    for (i = 0; i < nmbr_of_adc_inputs; i++) {
        if (adc_inputs[i].address == 3) {
            if (!(adc_inputs[i].is_valid_value)) {
                poll_auto_start = auto_start_error;
                return (0.0);
            }
            adc3_value = adc_inputs[i].value;
            break;
        }
    }

    /*
     * 3.0 term is VRef+ precision voltage reference
     * 1024.0 term is 10-bit ADC
     */
    diode_voltage = 3.0 * (adc3_value / 1024.0);
    
    struct diode_entry *diode_table = get_diode_table();

    if (diode_voltage > diode_table[0].voltage) {
        feedlog("overvoltage");
        poll_auto_start = auto_start_error;
        return (0.0);
    }

    for (i = 1; i < diode_N; i++) {
        if (diode_voltage >= diode_table[i].voltage) {
            break;
        }
    }

    if (!(i < diode_N)) {
        feedlog("undervoltage");
        poll_auto_start = auto_start_error;
        return (0.0);
    }

    // interpolate with line equation is good enough
    m = (diode_table[i].tempK - diode_table[i-1].tempK) /
            (diode_table[i].voltage - diode_table[i-1].voltage);
    b = diode_table[i].tempK - (m * diode_table[i].voltage);

    tempk = (m * diode_voltage) + b;

    sprintf(msg, "%03.1f", tempk);
    feedlog(msg);

    return (tempk);
}

bool get_auto_start_state() {
    return auto_start_state;
}

void set_auto_start_state(bool state) {
    auto_start_state = state;
}

void save_autostart_state() {
    FIL fp;
    FRESULT rslt;
    UINT bytes_written;

    char text_state[19];

    strcpy(text_state, (auto_start_state == true ? "auto" : "manual"));

    rslt = f_open(&fp, "STRTMODE.TXT", (FA_CREATE_ALWAYS | FA_WRITE));
    if (rslt == FR_OK) {
        f_write(&fp, text_state, strlen(text_state), &bytes_written);
        f_close(&fp);
    }
}

void load_autostart_state() {
    FIL fp;
    FRESULT rslt;
    UINT bytes_read;

    char text_state[19];

    auto_start_state = false;

    rslt = f_open(&fp, "STRTMODE.TXT", (FA_READ));
    if (rslt == FR_OK) {
        f_read(&fp, text_state, 6, &bytes_read);
        f_close(&fp);
        if (bytes_read >= 4) {
            text_state[bytes_read] = 0;
            if (strcasecmp(text_state, "auto") == 0) {
                auto_start_state = true;
            }
        }
    }
}
