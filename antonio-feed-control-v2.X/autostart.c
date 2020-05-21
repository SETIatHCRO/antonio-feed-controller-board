#include <stdint.h>         /* For uint32_t definition                        */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>        /* For true/false definition     */

#include "temperature.h"
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

//function accessible only here
static void autostartgetvactime_command(char *args[], int * timevar);
static void autostartsetvactime_command(char *args[], int * timevar,void (*savefun)(void));
static void load_autostart_vacuum_time( int * val, char * filename );
static void save_autostart_vacuum_time( int * val, char * filename );

extern bool update_logs;
extern bool is_parse_vacuum_response;

extern bool is_vac_response_ready;
extern bool is_vac_busy;

#define AUTOSTART_DEBUG_PRINT 0

extern bool is_cryo_busy;
extern bool is_cryo_response_ready;

extern char vac_request[MAX_VAC_COMMAND_LEN];
extern char vac_response[MAX_VAC_RESPONSE_LEN];
extern char cryo_response[MAX_CRYO_RESPONSE_LEN];

extern char *EOL;
extern char OK[];

struct oneshot_timer auto_start_timer = {"auto_start_timer", 0, NULL, NULL};

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#define AUTO_START_MAX_TRIES 3

int vacuum_autostart_backing_min = 10;
int vacuum_autostart_standby_min = 15;
int vacuum_autostart_turbo_min = 10;

//float autostart_highTemp = 285.0;
//float autostart_coold_start_threshold = 200.0;
float autostart_switchTemp = 130.0;
//float autostart_switch_limitLow = 65.0;
//float autostart_switch_limitHigh = 300.0;
//float autostart_cryo_safe_temp = 60.0; //C
int current_power_loop = 0;
int total_power_loops = 0;
int power_ramp_up_time_min = 6*60;
float power_loop_delta;
int rot_speed_test = 0;

/**
 * @brief autostart_machine_state the state that the autostart machine is at
 * stable states clears last 8 bits
 * errors are not cleared (except with clrerr command)
 * 0x000000 - not initialized (or manual autostart)
 * 0x000001 - started initialization
 * 0x000002 - started vacuum pumping
 * 0x000004 - init cooling
 * 0x000008 - cooling down - power
 * 0x000010 - cooling down - temp (reaching stable state clears previous bits)
 * 0x000020 - heating up
 * 0x000040 - switching off
 * 0x000080 - stable high temp (shutdown) state
 * 0x000100 - e000 occurred
 * 0x000200 - e001 occurred
 * 0x000400 - e002 occurred
 * 0x000800 - e003 occurred
 * 0x001000 - e004 occurred
 * 0x002000 - e005 occurred
 * 0x004000 - e006 occurred
 * 0x008000 - e007 occurred
 * 0x010000 - e008 occurred
 * 0x020000 - e009 occurred
 * 0x040000 - e010 occurred
 * 0x080000 - e011 occurred
 * 0x100000 - temp readout problem (A5/A6)
 * 0x200000 - e012 cryo failure in low state
 * 0x400000 - cryo comm problem (timeout)
 * 0x800000 - vac comm problem (timeout)
 */
int32_t autostart_machine_state = 0;

unsigned int auto_start_tries = 0;

unsigned int auto_start_cmnd_rspns_tries = 0;

char auto_start_request[AUTO_START_CMND_RSPNS_MAX_LEN];

char auto_start_response[AUTO_START_CMND_RSPNS_MAX_LEN];

bool auto_start_state = false;

bool should_report_complete = true;

bool autostart_cold_start=false;
bool doing_startup = false;
bool doing_shutdown = false;
bool autostart_vac_oscilating = false;
bool auto_start_manual_shutdown = false;
bool error_shutdown = false;
int fore_vacuum_try = 0;
int turbo_power_try = 0;
int standby_power_try = 0;
// TODO: embed scripting engine for auto start procedure

void auto_start_idle() {
    if (auto_start_state == true) {
        start_timer(&auto_start_timer, auto_start_timer_callback,
                AUTO_START_1_MIN);
        auto_start_next_state = auto_start_i000_request;
        poll_auto_start = auto_start_delay;
        send_to_rimbox("\r\nautostart in 1 minute\r\n");
        return;
    }
    if ( (auto_start_state == false ) && (auto_start_manual_shutdown==false) )
    {
        start_timer(&auto_start_timer, auto_start_timer_callback,
                AUTO_START_1_MIN);
        auto_start_next_state = auto_start_m001_request;
        poll_auto_start = auto_start_delay;
        send_to_rimbox("\r\nmanual shutdown 1 min\r\n");
        return;
    }
}

void auto_start_delay() {
    
}

void auto_start_timer_callback() {
    
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
    float tempk;
    doing_startup = false;
    if(should_report_complete) {
        autostart_machine_state &= 0xFFFFFF00;
        autostart_machine_state |= 0x00000010;
        autostart_cold_start=false;
        send_to_rimbox("\r\nautostart complete\r\n");
        should_report_complete = false;
        update_logs = false;
    }
    if(doing_shutdown) {
        poll_auto_start = auto_start_u001_request;
        should_report_complete = true;
        update_logs = true;
        return;
    }
    tempk = get_temp("A5");
    if ( (tempk == TEMP_NOT_INITIALIZED) || (tempk == TEMP_INVALID_TEMP) || (tempk == TEMP_INVALID_NAME) )
    {
        //we ignore it and just report error
        autostart_machine_state |= 0x00100000;
        feedlog_always("A5 failed");
    }
    if (tempk > AUTO_START_CRYO_SAFE_TEMP_C) {
        poll_auto_start = auto_start_e011;
        return;
    }
    tempk = get_temp("A6");
    if ( (tempk == TEMP_NOT_INITIALIZED) || (tempk == TEMP_INVALID_TEMP) || (tempk == TEMP_INVALID_NAME) )
    {
        //we ignore it and just report error
        autostart_machine_state |= 0x00100000;
        feedlog_always("A5 failed");
    }
    if (tempk > AUTO_START_CRYO_SAFE_TEMP_C) {
        poll_auto_start = auto_start_e011;
        return;
    }
    start_timer(&auto_start_timer, auto_start_timer_callback,AUTO_START_5_MIN);
    auto_start_next_state = auto_start_check_vacuum_request;
    poll_auto_start = auto_start_delay;
}

void shutdown_complete() {
    doing_shutdown = false;
    if(should_report_complete) {
        autostart_machine_state &= 0xFFFFFF00;
        autostart_machine_state |= 0x00000080;
        send_to_rimbox("\r\nshutdown complete\r\n");
        should_report_complete = false;
    }
    if (doing_startup) {
        poll_auto_start = auto_start_i000_request;
        should_report_complete = true;
    }
}

void auto_start_check_vacuum_request()
{
    autostart_generic_vacuum_request("p302", auto_start_check_vacuum_response);
}

void auto_start_check_vacuum_response()
{
    autostart_test_vacuum_param_true(auto_start_check_cryo_request,auto_start_e005,auto_start_e007);
}

void auto_start_check_cryo_request()
{
    autostart_generic_cryo_request("SET SSTOP", auto_start_check_cryo_response);
}

void auto_start_check_cryo_response()
{
    autostart_generic_cryo_response(0.0, auto_start_complete, auto_start_e012);
}

void clearautostarterr_command(char *args[])
{
    autostart_machine_state &= 0x000000FF;
    send_to_rimbox(OK);
    send_to_rimbox(EOL);
}

void autostart_command(char *args[])
{
    doing_startup = true;
    doing_shutdown = false;
    //if we were waiting for a timer - making sure timer ends in 3s
    //it's not the most graceful part of the code though
    if(auto_start_timer.countdown_ticks > 3000)
        auto_start_timer.countdown_ticks = 3000;
    send_to_rimbox(OK);
    send_to_rimbox(EOL);
}

void shutdown_command(char *args[])
{
    doing_startup = false;
    doing_shutdown = true;
    //if we were waiting for a timer - making sure timer ends in 3s
    //it's not the most graceful part of the code though
    if(auto_start_timer.countdown_ticks > 3000)
        auto_start_timer.countdown_ticks = 3000;
    send_to_rimbox(OK);
    send_to_rimbox(EOL);
}

void autostartgetbackingtime_command(char *args[])
{
    autostartgetvactime_command(args,&vacuum_autostart_backing_min);
}

void autostartgetstandbytime_command(char *args[])
{
    autostartgetvactime_command(args,&vacuum_autostart_standby_min);
}

void autostartgetturbotime_command(char *args[])
{
    autostartgetvactime_command(args,&vacuum_autostart_turbo_min);
}

static void autostartgetvactime_command(char *args[], int * timevar)
{
    char msg[19];
    snprintf(msg, 18, "\r%d", *timevar);
    send_to_rimbox(msg);
    send_to_rimbox(EOL);
}

void autostartsetbackingtime_command(char *args[])
{
    autostartsetvactime_command(args,&vacuum_autostart_backing_min,save_autostart_backing_time);
}

void autostartsetstandbytime_command(char *args[])
{
    autostartsetvactime_command(args,&vacuum_autostart_standby_min,save_autostart_standby_time);
}

void autostartsetturbotime_command(char *args[])
{
    autostartsetvactime_command(args,&vacuum_autostart_turbo_min,save_autostart_turbo_time);
}

static void autostartsetvactime_command(char *args[], int * timevar,void (*savefun)(void))
{
    //char msg[19];
    int N;
    int next_vac_time;
    if (args[0] == NULL) {
        send_to_rimbox(EOL);
        return;
    }

    N = sscanf(args[0],"%d",&next_vac_time);
    if (!N) {
        //snprintf(msg, 18, "\rBad N(%d)%s\r\n",N,args[0]);
        //send_to_rimbox(msg);
        send_to_rimbox(EOL);
        return;
    }
    if( (next_vac_time < AUTO_START_MIN_VAC_TIME_MIN ) || ( next_vac_time > AUTO_START_MAX_VAC_TIME_MIN ) )
    {
        //snprintf(msg, 18, "\rHERE%3.2f\r\n", next_temp);
        //send_to_rimbox(msg);
        send_to_rimbox(EOL);
        return;
    }
    *timevar = next_vac_time;
    savefun();
    send_to_rimbox(OK);
    send_to_rimbox(EOL);
}

void getswitchtemp_command(char *args[])
{
    char msg[19];
    snprintf(msg, 18, "\r%3.2f", autostart_switchTemp);
    send_to_rimbox(msg);
    send_to_rimbox(EOL);
}

void setswitchtemp_command(char *args[])
{
    //char msg[19];
    int N;
    float next_temp;
    if (args[0] == NULL) {
        send_to_rimbox(EOL);
        return;
    }
    
    char * foo;
    float tmpval;
    tmpval = strtof(args[0],&foo);
    sscanf(args[0],"%f",&next_temp);
    N = !(foo == args[0]);
    //N = sscanf(args[0], "%f", &next_temp);
    if (!N) {
        //snprintf(msg, 18, "\rBad N(%d)%s\r\n",N,args[0]);
        //send_to_rimbox(msg);
        send_to_rimbox(EOL);
        return;
    }
    if( (next_temp < AUTO_START_SWITCH_TEMP_LIMIT_LOW_K ) || (next_temp > AUTO_START_SWITCH_TEMP_LIMIT_HIGH_K ) )
    {
        //snprintf(msg, 18, "\rHERE%3.2f\r\n", next_temp);
        //send_to_rimbox(msg);
        send_to_rimbox(EOL);
        return;
    }
    autostart_switchTemp = next_temp;
    save_autostart_switchTemp();
    send_to_rimbox(OK);
    send_to_rimbox(EOL);
}

void autostartgetstate_command(char *args[])
{
    char msg[19];
    snprintf(msg, 18, "\r0x%x", autostart_machine_state);
    send_to_rimbox(msg);
    send_to_rimbox(EOL);
}

void autostart_generic_vacuum_request(char* vac_cmd, void (*next_fun)(void)) {
    //strcpy(auto_start_request, vac_cmd);
    strncpy(auto_start_request, vac_cmd, AUTO_START_CMND_RSPNS_MAX_LEN-1);

#if AUTOSTART_DEBUG_PRINT
    char msg[31];
    snprintf(msg, 30, "dbgvcs:%s\n\r\n",vac_cmd);
    send_to_rimbox(msg);
#endif

    auto_start_cmnd_rspns_tries = 0;

    auto_start_next_state = next_fun;
    poll_auto_start = auto_start_send_request_to_vac;
}

void autostart_generic_vacuum_response(char* vac_resp, void (*next_fun)(void), void (*err_fun)(void)) {

#if AUTOSTART_DEBUG_PRINT
    char msg[31];
    snprintf(msg, 30, "dbgvcr:%s\n\r\n",auto_start_response);
    send_to_rimbox(msg);
#endif

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
#if AUTOSTART_DEBUG_PRINT
    char msg[31];
    snprintf(msg, 30, "dbgvct:%s\n\r\n",auto_start_response);
    send_to_rimbox(msg);
#endif
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
#if AUTOSTART_DEBUG_PRINT
    char msg[31];
    snprintf(msg, 30, "dbgcrs:%s\n\r\n",cryo_cmd);
    send_to_rimbox(msg);
#endif

    auto_start_cmnd_rspns_tries = 0;

    auto_start_next_state = next_fun;
    poll_auto_start = auto_start_send_request_to_cryo;
}

int autostart_vac_getulongfromresp(unsigned long int * val)
{
    unsigned long int tmpval;
#if AUTOSTART_DEBUG_PRINT
    char msg[31];
    
    snprintf(msg, 30, "dbgvcu:%s\n\r\n",auto_start_response);
    send_to_rimbox(msg);
#endif
    int NN;
    //tmpval = strtoul(auto_start_response,&foo);
    //TODO: i haven't try to investigate whan NN value will be set to if 
    //response is not represented by a string
    NN = !sscanf(auto_start_response,"%lu",val);
#if AUTOSTART_DEBUG_PRINT
    //snprintf(msg, 30, "dbgvcu:got %u(%d)\n\r\n",*val,auto_start_response == foo);
    snprintf(msg, 30, "dbgvcu:got %u(%d)\n\r\n",*val,NN);
    send_to_rimbox(msg);
#endif
    
    return NN;
    //return (foo == auto_start_response);
}

int autostart_cryo_getfloatfromresp(float * val)
{
    char * crsp;
    char *foo;
    *val = 0;
    float tmpval;
    
#if AUTOSTART_DEBUG_PRINT
    char msg[31];
    
    snprintf(msg, 30, "dbgcrf:%s\n\r\n",auto_start_response);
    send_to_rimbox(msg);
#endif
    
    crsp = strtok(auto_start_response, "\r\n");
    if (crsp == NULL) {
        //there is no first line
        return 1;
    }
    
    crsp = strtok(NULL,"\r\n");
    if (crsp == NULL) {
        //there is no second line
        return 1;
    }
    tmpval = strtof(crsp,&foo);
    sscanf(crsp,"%f",val);
#if AUTOSTART_DEBUG_PRINT
    snprintf(msg, 30, "dbgcrf:got '%s'\n\r\n",crsp);
    send_to_rimbox(msg);
    snprintf(msg, 30, "dbgcrf:got %.2f(%d)\n\r\n",*val,crsp == foo);
    send_to_rimbox(msg);
#endif
    //if those are equal, that means the conversion failed
    return (crsp == foo);
}

void autostart_generic_cryo_response(float cryo_resp, void (*next_fun)(void), void (*err_fun)(void))
{
    float cryo_flt_rspns;

    int Ncr = 0;
    Ncr = !autostart_cryo_getfloatfromresp(&cryo_flt_rspns);
    
#if AUTOSTART_DEBUG_PRINT
        char msg[31];
        
        snprintf(msg, 30, "dbgcrq:%d_%3.2f_%3.2f\n\r\n",Ncr,cryo_flt_rspns,cryo_resp);
        send_to_rimbox(msg);
#endif
    if ((Ncr) && (cryo_flt_rspns > cryo_resp-0.1) && (cryo_flt_rspns <  cryo_resp+0.1)) {
#if AUTOSTART_DEBUG_PRINT
        snprintf(msg, 30, "dbgcr2:%d_%3.2f_%3.2f\n\r\n",Ncr,cryo_flt_rspns,cryo_resp);
        send_to_rimbox(msg);
#endif
        poll_auto_start = next_fun;
        return;
    }

    auto_start_cmnd_rspns_tries += 1;

    if (auto_start_cmnd_rspns_tries < AUTO_START_CMND_RSPNS_MAX_TRIES) {
#if AUTOSTART_DEBUG_PRINT
        snprintf(msg, 30, "dbgcr3:resending\n\r\n");
        send_to_rimbox(msg);
#endif        
        poll_auto_start = auto_start_send_request_to_cryo;
        return;
    }

    poll_auto_start = err_fun;
}

void autostart_test_vacuum_param_true(void (*true_fun)(void), void (*false_fun)(void), void (*err_fun)(void))
{
    unsigned long int resp_val;
    int N;
    //char * foo;
    //resp_val = strtoul(auto_start_response,&foo);
    //N = !(foo == auto_start_response);
    N = !autostart_vac_getulongfromresp(&resp_val);
    //int N = sscanf(auto_start_response, "%u", &resp_val);

    if (N) {
        if (resp_val == 0) {
            poll_auto_start = false_fun;
            return;
        } else {
            poll_auto_start = true_fun;
            return;
        }
    }
    auto_start_cmnd_rspns_tries += 1;

    if (auto_start_cmnd_rspns_tries < AUTO_START_CMND_RSPNS_MAX_TRIES) {
        poll_auto_start = auto_start_send_request_to_vac;
        return;
    }
    poll_auto_start = err_fun;
}

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
    //char *rspnsln;

    if (!(is_cryo_response_ready)) {
        return;
    }

    //we are in fact interested in second line
    /*rspnsln = strtok(cryo_response, "\r");
    if (rspnsln == NULL) {
        strcpy (auto_start_response, "");
    }
    else {
        strcpy (auto_start_response, rspnsln);
    }*/
    //instead we want to copy all string, if \r is in cryo_response
    cryo_response[MAX_CRYO_RESPONSE_LEN-1] = '\0';
    if(strchr(cryo_response,'\r') == NULL) {
        strncpy(auto_start_response, "",AUTO_START_CMND_RSPNS_MAX_LEN-1);
    } else {
        strncpy(auto_start_response,cryo_response,AUTO_START_CMND_RSPNS_MAX_LEN-1);
    }
    auto_start_response[AUTO_START_CMND_RSPNS_MAX_LEN-1] = '\0';
    feedlog(auto_start_response);

    free_cryo_session();

    poll_auto_start = auto_start_next_state;
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

void save_autostart_backing_time()
{
    save_autostart_vacuum_time(&vacuum_autostart_backing_min, "VCBKTIME.TXT");
}

void save_autostart_standby_time()
{
    save_autostart_vacuum_time(&vacuum_autostart_standby_min, "VCSBTIME.TXT");
}

void save_autostart_turbo_time()
{
    save_autostart_vacuum_time(&vacuum_autostart_turbo_min, "VCTBTIME.TXT");
}

static void save_autostart_vacuum_time( int * val, char * filename )
{
    FIL fp;
    FRESULT rslt;
    UINT bytes_written;

    char text_state[19];

    snprintf(text_state,18,"%d",*val);

    rslt = f_open(&fp, filename, (FA_CREATE_ALWAYS | FA_WRITE));
    if (rslt == FR_OK) {
        f_write(&fp, text_state, strlen(text_state), &bytes_written);
        f_close(&fp);
    }
}

void load_autostart_backing_time()
{
    load_autostart_vacuum_time(&vacuum_autostart_backing_min, "VCBKTIME.TXT");
}

void load_autostart_standby_time()
{
    load_autostart_vacuum_time(&vacuum_autostart_standby_min, "VCSBTIME.TXT");
}

void load_autostart_turbo_time()
{
    load_autostart_vacuum_time(&vacuum_autostart_turbo_min, "VCTBTIME.TXT");
}

static void load_autostart_vacuum_time( int * val, char * filename )
{
    FIL fp;
    FRESULT rslt;
    UINT bytes_read;
    int next_vac_time = 0;

    char text_state[19];

    rslt = f_open(&fp, filename, (FA_READ));
    if (rslt == FR_OK) {
        f_read(&fp, text_state, 10, &bytes_read);
        f_close(&fp);
        if (bytes_read > 0) {
            text_state[bytes_read] = 0;
            sscanf(text_state,"%d",&next_vac_time);
            if (( next_vac_time >= AUTO_START_MIN_VAC_TIME_MIN ) && ( next_vac_time <= AUTO_START_MAX_VAC_TIME_MIN ) )
            {
                *val = next_vac_time;
            }
        }
    }
}

void save_autostart_switchTemp() {
    FIL fp;
    FRESULT rslt;
    UINT bytes_written;

    char text_state[19];

    snprintf(text_state,18,"%3.2f",autostart_switchTemp);

    rslt = f_open(&fp, "STRSTEMP.TXT", (FA_CREATE_ALWAYS | FA_WRITE));
    if (rslt == FR_OK) {
        f_write(&fp, text_state, strlen(text_state), &bytes_written);
        f_close(&fp);
    }
}

void load_autostart_switchTemp()
{
    FIL fp;
    FRESULT rslt;
    UINT bytes_read;
    float next_temp;

    char text_state[19];

    rslt = f_open(&fp, "STRSTEMP.TXT", (FA_READ));
    if (rslt == FR_OK) {
        f_read(&fp, text_state, 10, &bytes_read);
        f_close(&fp);
        if (bytes_read > 0) {
            text_state[bytes_read] = 0;
            sscanf(text_state,"%f",&next_temp);
            if ((next_temp >= AUTO_START_SWITCH_TEMP_LIMIT_LOW_K) && (next_temp <= AUTO_START_SWITCH_TEMP_LIMIT_HIGH_K) )
            {
                autostart_switchTemp = next_temp;
            }
        }
    }
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
