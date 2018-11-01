/******************************************************************************/
/*  Files to Include                                                          */
/******************************************************************************/
#ifdef __XC32
    #include <xc.h>          /* Defines special funciton registers, CP0 regs  */
#endif

#include <plib.h>           /* Include to use PIC32 peripheral libraries      */
#include <stdio.h>
#include <stdint.h>         /* For uint32_t definition                        */
#include <stdbool.h>        /* For true/false definition     */
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "system.h"         /* System funct/params, like osc/periph config    */
#include "user.h"           /* User funct/params, such as InitApp             */
#include "base64.h"
#include "adler.h"
#include "fatfs/diskio.h"
#include "fatfs/ff.h"
#include "oneshot.h"
#include "accel.h"
#include "tc74.h"
#include "adc.h"

/******************************************************************************/
/* Global Variable Declaration                                                */
/******************************************************************************/

#undef MYDEBUG

#undef MYFANSENSEDEBUG
#undef MYDISKIODEBUG

#undef MYFANAUTODEBUG

float version = 3.12;

static char init_string[] = "Antonio Feed Monitor and Control Firmware";

/*
 * watchdog support ...................................................
 */

bool is_rimbox_session_inactive = false;

struct oneshot_timer rimbox_session_timeout_timer =
    {"rimbox_session_timeout_timer", 0, NULL, NULL};

/*
 * feed auto start support ............................................
 */

#define AUTO_START_1_MIN 60*1000
#define AUTO_START_15_MIN 15*60*1000
#define AUTO_START_45_MIN 45*60*1000

#define AUTO_START_GETDIODE_THRESHOLD 250.0

#define AUTO_START_MAX_TRIES 3

#define AUTO_START_CMND_RSPNS_MAX_TRIES 2

#define AUTO_START_CMND_RSPNS_MAX_LEN 99

unsigned int auto_start_tries = 0;

unsigned int auto_start_cmnd_rspns_tries = 0;

char auto_start_request[AUTO_START_CMND_RSPNS_MAX_LEN];

char auto_start_response[AUTO_START_CMND_RSPNS_MAX_LEN];

bool auto_start_state = false;

void (*poll_auto_start)();

void (*auto_start_next_state)();
void (*auto_start_save_next_state)();

struct oneshot_timer auto_start_timer = {"auto_start_timer", 0, NULL, NULL};

// auto start procedures are implemented at end of main.c

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

/*
 *
 */

const float max_reject_temp = 70.0;
const float hot_reject_temp = 55.0;
const float warm_reject_temp = 40.0;
const float cold_reject_temp = 0.0;

const float max_case_temp = 80.0;
const float hot_case_temp = 65.0;
const float warm_case_temp = 50.0;
const float cold_case_temp = 0.0;

const unsigned int vac_power_threshold = 25;

const unsigned int auto_start_retries = 3;

/*
 * 4-wire fan support .................................................
 */

#define FAN_MODE_AUTO   0
#define FAN_MODE_MANUAL 1

unsigned int fan_mode = FAN_MODE_MANUAL;

const unsigned int min_fan_pwm = 20;
const unsigned int cold_fan_pwm = 30;
const unsigned int warm_fan_pwm = 66;
const unsigned int default_fan_pwm = 90;
const unsigned int hot_fan_pwm = 100;

unsigned int fan_pwm_percent = 90;

struct oneshot_timer fan_speed_timer = {"fan_speed_timer", 0, NULL, NULL};
struct oneshot_timer fan_sense_timeout_timer = {"fan_sense_timeout", 0, NULL, NULL};

uint16_t fan_sense_previous_captured_value = 0;

float fan_pulses_per_rotation = 2;

unsigned int fan_rpm = 0;

void (*fan_sense_state)();

unsigned int auto_fan_reject_tc74_i = 5;
unsigned int auto_fan_case_tc74_i = 6;

//unsigned int auto_fan_reject_tc74_i = 2;
//unsigned int auto_fan_case_tc74_i = 3;

/*
 * relay support
 */

bool relay_state = false;  // false is not energized

/*
 * cryo diode support
 */

#define NUM_DIODE_ENTRIES 99

unsigned int diode_N = 35;

struct diode_entry {
    float tempK;
    float voltage;
};

//// init to compromize between DT-470 and DT-670
//struct diode_entry diode_table[NUM_DIODE_ENTRIES] = {
//    {10,   1.402},
//    {20,   1.206},
//    {30,   1.107},
//    {40,   1.088},
//    {50,   1.072},
//    {60,   1.055},
//    {70,   1.037},
//    {80,   1.019},
//    {90,   1.000},
//    {100,  0.981},
//    {110,  0.962},
//    {120,  0.941},
//    {130,  0.921},
//    {140,  0.900},
//    {150,  0.879},
//    {160,  0.858},
//    {170,  0.836},
//    {180,  0.814},
//    {190,  0.792},
//    {200,  0.770},
//    {210,  0.747},
//    {220,  0.725},
//    {230,  0.702},
//    {240,  0.679},
//    {250,  0.656},
//    {260,  0.633},
//    {270,  0.610},
//    {280,  0.586},
//    {290,  0.563},
//    {300,  0.539},
//    {310,  0.516},
//    {320,  0.492},
//    {330,  0.468},
//    {340,  0.444},
//    {350,  0.421}
//};

// DT-470
struct diode_entry diode_table[NUM_DIODE_ENTRIES] = {
    {10,   1.420},
    {20,   1.214},
    {30,   1.107},
    {40,   1.088},
    {50,   1.071},
    {60,   1.053},
    {70,   1.034},
    {80,   1.015},
    {90,   0.996},
    {100,  0.976},
    {110,  0.955},
    {120,  0.934},
    {130,  0.912},
    {140,  0.891},
    {150,  0.869},
    {160,  0.847},
    {170,  0.824},
    {180,  0.801},
    {190,  0.779},
    {200,  0.756},
    {210,  0.732},
    {220,  0.709},
    {230,  0.686},
    {240,  0.662},
    {250,  0.638},
    {260,  0.615},
    {270,  0.591},
    {280,  0.567},
    {290,  0.543},
    {300,  0.519},
    {310,  0.495},
    {320,  0.471},
    {330,  0.446},
    {340,  0.422},
    {350,  0.398}
};

/*
 * rimbox communications and message level support (commands further below)
 */

char CR[]   = "\r";
char CRLF[] = "\r\n";
char VBAR[] = "|";

char *LINESEP = VBAR;
char *EOL = CR;

bool echo_mode = false;

#define MAX_COMMAND_LEN 999
#define MAX_RESPONSE_LEN 999

#define MAX_ARGS 99

unsigned int rimbox_cmnd_i = 0;
char rimbox_command[MAX_COMMAND_LEN];
char *command;
char *args[MAX_ARGS];

#define RIMBOX_SEND_FIFO_SIZE 1999

unsigned int rimbox_send_head_i = 0;
unsigned int rimbox_send_tail_i = 0;
unsigned int rimbox_send_count = 0;
// for this FIFO, characters enter at the head and leave by the tail
char rimbox_send_fifo[RIMBOX_SEND_FIFO_SIZE];

void (*poll_recv_from_rimbox)();

/*
 * vacuum drive communications and message level support ..............
 */

unsigned int vac_req_i = 0;
char vac_request[MAX_COMMAND_LEN];

unsigned int vac_rspns_i = 0;
char vac_response[MAX_RESPONSE_LEN];

bool is_vac_busy = false;

bool is_vac_response_ready = false;

void (*poll_vac_session)();

struct oneshot_timer vac_rspns_timeout_timer = {"vac_response_timeout", 0, NULL, NULL};

unsigned int vacuum_address = 1;

bool is_parse_vacuum_response = false;

/*
 * cryo controller communications and message level support ...........
 */

unsigned int cryo_req_i = 0;
char cryo_request[MAX_COMMAND_LEN];

unsigned int cryo_rspns_i = 0;
char cryo_response[MAX_RESPONSE_LEN];

bool is_cryo_busy = false;

bool is_cryo_response_ready = false;

void (*poll_cryo_session)();

struct oneshot_timer cryo_rspns_timeout_timer = {"cryo_response_timeout", 0, NULL, NULL};
struct oneshot_timer cryo_rspns_eol_timer = {"cryo_eol_timeout", 0, NULL, NULL};

/*
 * external flash filesystem support ..................................
 */

FATFS fso;

/*
 * internal commands support ..........................................
 */

void help_command(char *args[]);

void setfanpwm_command(char *args[]);
void getfanpwm_command(char *args[]);
void getfanrpm_command(char *args[]);
void gettemp_command(char *args[]);
void getvacuum_command(char *args[]);
void getdiode_command(char *args[]);
void getaccel_command(char *args[]);
void setrelay_command(char *args[]);
void getrelay_command(char *args[]);
void ls_command(char *args[]);
void cat_command(char *args[]);
void get24v_command(char *args[]);
void get48v_command(char *args[]);
void getcryoattemp_command(char *args[]);
void setfeedstartmode_command(char *args[]);
void getfeedstartmode_command(char *args[]);
void hyperterminal_command(char *args[]);
void stty_command(char *args[]);
void getversion_command(char *args[]);
void bootloader_command(char *args[]);
void reset_command(char *args[]);

typedef void (*command_function) (char *args[]);

struct command_pair {
    char *name;
    command_function function;
    char *helpfile;
};

/*
 * associate command names with their functions and help files
 */
struct command_pair commands[] = {
    {"help", help_command, NULL},
    {"setfanpwm", setfanpwm_command, "sfpwm.txt"},
    {"getfanpwm", getfanpwm_command, "gfpwm.txt"},
    {"getfanrpm", getfanrpm_command, "gfrpm.txt"},
    {"getfanspeed", getfanrpm_command, "gfrpm.txt"},
    {"gettemp", gettemp_command, "gettemp.txt"},
    {"gt", gettemp_command, "gettemp.txt"},
    {"getvacuum", getvacuum_command, "getvac.txt"},
    {"getvac", getvacuum_command, "getvac.txt"},
    {"gv", getvacuum_command, "getvac.txt"},
    {"getdiode", getdiode_command, "getdiode.txt"},
    {"gd", getdiode_command, "getdiode.txt"},
    {"getaccel", getaccel_command, "getaccel.txt"},
    {"setrelay", setrelay_command, "setrelay.txt"},
    {"getrelay", getrelay_command, "getrelay.txt"},
    {"ls", ls_command, "ls.txt"},
    {"dir", ls_command, "ls.txt"},
    {"cat", cat_command, "cat.txt"},
    {"type", cat_command, "cat.txt"},
    {"get24v", get24v_command, "get24v.txt"},
    {"get48v", get48v_command, "get48v.txt"},
    {"getcryoattemp", getcryoattemp_command, "getattmp.txt"},
    {"setfeedstartmode", setfeedstartmode_command, "setstart.txt"},
    {"getfeedstartmode", getfeedstartmode_command, "getstart.txt"},
    {"hyperterminal", hyperterminal_command, "hyprterm.txt"},
    {"ht", hyperterminal_command, "hyprterm.txt"},
    {"minex", hyperterminal_command, "hyprterm.txt"},
    {"stty", stty_command, "stty.txt"},
    {"rimbox", stty_command, "stty.txt"},
    {"getversion", getversion_command, "getversn.txt"},
    {"bootloader", bootloader_command, "bootload.txt"},
    {"reset", reset_command, "reset.txt"}
};

#define NUMBER_OF_COMMANDS (sizeof (commands) / sizeof(struct command_pair))

char OK[] = "OK";
char ERROR[] = "error";
char TIMEOUT[] = "timeout";
//char ACK[] = "ACK";
//char NAK[] = "NAK";

// main function definitions ..........................................

void toggle_watchdog();
void watchdog_rimbox_session_timeout();

void delay_and_turn_on_i2c();

void purge_chars_from_rimbox();
void recv_cmnd_from_rimbox();
void parse_cmnd_from_rimbox();
bool is_processed_short_vacuum_command(char *command);
void compute_checksum_if_requested(char *command);
bool is_command_uppercase(char *command);
void wait_vacuum_not_busy();
void send_command_to_vacuum(char *command);
void wait_for_vacuum_response();
void free_vacuum_session();
void parse_vacuum_response(char *response);
void wait_cryo_not_busy();
void send_command_to_cryo(char *command, char *args[]);
void wait_for_cryo_response();
void free_cryo_session();

void poll_send_to_rimbox();

void send_to_rimbox(char *msg);
void send_char_to_rimbox(char c);

void vac_poll_idle();
void vac_poll_send_request();
void vac_poll_get_response();

void vac_response_timeout();

void cryo_poll_idle();
void cryo_poll_send_request();
void cryo_poll_get_response_first_line();
void cryo_poll_get_response_remaining_lines();

void cryo_response_timeout();
void cryo_response_eol_timeout();

void set_fan_speed_callback();

#ifdef MYFANAUTODEBUG
void test_fan_speed_auto();
#endif

void set_fan_speed_auto();
unsigned int compute_fan_pwm_from_reject_temp(float reject_temp_c);
unsigned int compute_fan_pwm_from_case_temp(float case_temp_c);

void set_fan_pwm(unsigned int percent);

void poll_fan_sense();

void fan_sense_timeout_callback();

void fan_sense_init();
void fan_sense_wait();
void fan_sense_read();

void load_init();

void load_fan_state();
void save_fan_state();

void load_relay_state();
void save_relay_state();

void load_autostart_state();
void save_autostart_state();

void init_log();
void feedlog(char *msg);

#ifdef MYDEBUG
void mylog (char *msg);
#endif

/******************************************************************************/
/* Main Program                                                               */
/******************************************************************************/

int32_t main(void)
{
    char msg[99];
    int i;

    FRESULT res;

    uint32_t start_time;
    uint32_t end_time;

    /*Refer to the C32 peripheral library documentation for more
    information on the SYTEMConfig function.
    
    This function sets the PB divider, the Flash Wait States, and the DRM
    /wait states to the optimum value.  It also enables the cacheability for
    the K0 segment.  It could has side effects of possibly alter the pre-fetch
    buffer and cache.  It sets the RAM wait states to 0.  Other than
    the SYS_FREQ, this takes these parameters.  The top 3 may be '|'ed
    together:
    
    SYS_CFG_WAIT_STATES (configures flash wait states from system clock)
    SYS_CFG_PB_BUS (configures the PB bus from the system clock)
    SYS_CFG_PCACHE (configures the pCache if used)
    SYS_CFG_ALL (configures the flash wait states, PB bus, and pCache)*/

    /* TODO Add user clock/system configuration code if appropriate.  */

//  SYSTEMConfig(SYS_FREQ, SYS_CFG_ALL);
    // changed so that PBCLK defined by configuration bits
    SYSTEMConfig(SYS_FREQ, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);

    OpenCoreTimer(CORE_TIMER_PERIOD);
    mConfigIntCoreTimer((CT_INT_ON | CT_INT_PRIOR_2 | CT_INT_SUB_PRIOR_0));

    /*Configure Multivector Interrupt Mode.  Using Single Vector Mode
    is expensive from a timing perspective, so most applications
    should probably not use a Single Vector Mode*/
    INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);

    INTEnableSystemMultiVectoredInt();  // includes INTConfigureSystem() ?

    // delay for three seconds before getting started.  This prevents
    // annoying situation caused when microcontroller is reset multiple
    // times during programming.
    start_time = GetTickCount();
    do {
        end_time = GetTickCount();
    }
    while ((end_time - start_time) < 3000);

    /* Initialize I/O and Peripherals for application */
    InitApp();

    delay_and_turn_on_i2c();

    init_oneshot();

    poll_recv_from_rimbox = purge_chars_from_rimbox;
    poll_cryo_session = cryo_poll_idle;
    poll_vac_session = vac_poll_idle;
    poll_auto_start = auto_start_idle;

    res = f_mount(&fso, "", 1);  // mount the external flash filesystem
#ifdef MYDISKIODEBUG
    sprintf(msg, "f_mount(&fso, "", 1) returned %d", res);
    mylog(msg);
#endif
    init_log();

    init_tc74();
    
    init_adc();

    start_timer(&fan_speed_timer, set_fan_speed_callback, 10000);

    fan_sense_state = fan_sense_init;

#ifdef MYDEBUG
    mylog (EOL);
    mylog (EOL);
#endif

#ifdef MYADCDEBUG
    dump_adc_table();
#endif

    EnableWDT();

    sprintf(msg, "%s %4.2f", init_string, version);
    send_to_rimbox(msg);
    send_to_rimbox(EOL);
    feedlog(msg);

    load_init();

    load_relay_state();

    load_fan_state();

    load_autostart_state();

    sprintf(msg, "feed start mode is %s",
        (auto_start_state == true ? "automatic": "manual"));
    feedlog(msg);

#ifdef MYFANAUTODEBUG
//    test_fan_speed_auto();
#endif

    init_accel();

    while(1) {
        toggle_watchdog();

        poll_recv_from_rimbox();
        poll_accel();
        poll_oneshot_timers();

        poll_send_to_rimbox();
        poll_accel();
        poll_oneshot_timers();

        poll_vac_session();
        poll_accel();
        poll_oneshot_timers();

        poll_cryo_session();
        poll_accel();
        poll_oneshot_timers();

        poll_vac_session();
        poll_accel();
        poll_oneshot_timers();

        poll_cryo_session();
        poll_accel();
        poll_oneshot_timers();        

        poll_accel();
        poll_oneshot_timers();

        poll_adc();
        poll_accel();
        poll_oneshot_timers();

        poll_tc74();
        poll_accel();
        poll_oneshot_timers();

        poll_fan_sense();
        poll_accel();
        poll_oneshot_timers();

        poll_auto_start();
    }
}

void toggle_watchdog() {
        if (is_rimbox_session_inactive) {
            return;
        }
    
        ClearWDT();
}

void watchdog_rimbox_session_timeout() {
    is_rimbox_session_inactive = true;
}

void delay_and_turn_on_i2c() {
    uint32_t start_count = GetTickCount();

    mPORTEClearBits(BIT_4);  // I2C power off

    while ((GetTickCount() - start_count) < 500) {
    }

    mPORTESetBits(BIT_4);  // I2C power on

    start_count = GetTickCount();

    while ((GetTickCount() - start_count) < 100) {
    }
}

void purge_chars_from_rimbox() {
    char rimbox_recv_char;

    if (U1STA & 0x000E) {
        rimbox_recv_char = U1RXREG;
        U1STAbits.OERR = 0;
    }

    if (UARTReceivedDataIsAvailable(UART1)) {
        UARTGetDataByte(UART1);
        return;
    }

    rimbox_cmnd_i = 0;

    poll_recv_from_rimbox = recv_cmnd_from_rimbox;
}

void recv_cmnd_from_rimbox() {
    char rimbox_recv_char;

    if (U1STA & 0x000E) {
        rimbox_recv_char = U1RXREG;
        U1STAbits.OERR = 0;
    }

    if (UARTReceivedDataIsAvailable(UART1)) {
        rimbox_recv_char = UARTGetDataByte(UART1);
        if (rimbox_recv_char == 0x08) {  // backspace
            if (rimbox_cmnd_i > 0) {
                rimbox_cmnd_i--;
            }
            if (echo_mode) {
                send_char_to_rimbox(rimbox_recv_char);
                send_char_to_rimbox(' ');
                send_char_to_rimbox(rimbox_recv_char);
            }
            return;
        }
        if (rimbox_cmnd_i < MAX_COMMAND_LEN) {
            rimbox_command[rimbox_cmnd_i] = rimbox_recv_char;
            rimbox_cmnd_i = rimbox_cmnd_i + 1;
        }
        if ((rimbox_recv_char == 0x0D) || (rimbox_recv_char == 0x0A)) {
            rimbox_command[rimbox_cmnd_i - 1] = 0;  // terminate command string
            poll_recv_from_rimbox = parse_cmnd_from_rimbox;
            if (echo_mode) {
                send_to_rimbox(EOL);
            }
            return;
        }
        if (echo_mode) {
            send_char_to_rimbox(rimbox_recv_char);
        }
    }
}

void parse_cmnd_from_rimbox() {

    unsigned int i;
    unsigned int cmnds_i;
//    char *command;
//    char *args[MAX_ARGS];

    unsigned int args_i = 0;

    command = strtok(rimbox_command, " ");
    if (command == NULL) {
        poll_recv_from_rimbox = purge_chars_from_rimbox;
        return;
    }

    args[args_i] = strtok(NULL, " ");
    while (!(args[args_i] == NULL)) {
        args_i = args_i + 1;
        args[args_i] = strtok(NULL, " ");
    }

    for (cmnds_i = 0; cmnds_i < NUMBER_OF_COMMANDS; cmnds_i++) {
        if (strcmp(command, commands[cmnds_i].name) == 0) {
            commands[cmnds_i].function(args);
            poll_recv_from_rimbox = purge_chars_from_rimbox;
            return;
        }
    }

    if (is_processed_short_vacuum_command(command)) {
        poll_recv_from_rimbox = wait_vacuum_not_busy;
        return;
    }

    if (strlen(command) > 3) {
        if (isdigit(command[0]) && isdigit(command[1]) && isdigit(command[2])) {
            compute_checksum_if_requested(command);
            poll_recv_from_rimbox = wait_vacuum_not_busy;
            return;
        }
    }

    // uppercase commands (e.g., SET TTARGET) are routed to the cryo controller
    if (is_command_uppercase(command)) {
        poll_recv_from_rimbox = wait_cryo_not_busy;
        return;
    }

    // parse JSON request here ...
    // UPDATE: doesn't look like we are going to do a JSON interface

    send_to_rimbox(EOL);
    poll_recv_from_rimbox = purge_chars_from_rimbox;
}

bool is_processed_short_vacuum_command(char *command) {
    char address_string[9];
    char length_string[9];
    char vacuum_command[99];

    if (strlen(command) == 4) {
        if (command[0] == 'p') {
            if (isdigit(command[1]) && isdigit(command[2]) && isdigit(command[3])) {
                sprintf(address_string, "%03u", vacuum_address);
                strcpy(vacuum_command, address_string);
                strcat(vacuum_command, "00");
                strcat(vacuum_command, &command[1]);
                strcat(vacuum_command, "02=?###");
                compute_checksum_if_requested(vacuum_command);
                is_parse_vacuum_response = true;
                strcpy(command, vacuum_command);
                return (true);
            }
        }
    }

    if (strlen(command) > 5) {
        if ((command[0] == 'p') && (command[4] == '=')) {
            if (isdigit(command[1]) && isdigit(command[2]) && isdigit(command[3])) {
                sprintf(address_string, "%03u", vacuum_address);
                strcpy(vacuum_command, address_string);
                strcat(vacuum_command, "10");
                command[4] = 0;
                strcat(vacuum_command, &command[1]);
                sprintf(length_string, "%02u", strlen(&command[5]));
                strcat(vacuum_command, length_string);
                strcat(vacuum_command, &command[5]);
                strcat(vacuum_command, "###");
                compute_checksum_if_requested(vacuum_command);
                is_parse_vacuum_response = true;
                strcpy(command, vacuum_command);
                return (true);
            }
        }
    }

    is_parse_vacuum_response = false;
    
    return (false);
}

void compute_checksum_if_requested(char *command) {
    unsigned int i;
    char checksum_digits[9];
    unsigned int N = strlen(command);
    uint8_t checksum = 0;

    if (!(N > 3)) {
        return;
    }

    if ((command[N-3] == '#') && (command[N-2] == '#') &&
            (command[N-1] == '#')) {
        for (i = 0; i < (N - 3); i++) {
            checksum = checksum + command[i];
        }
        sprintf(checksum_digits, "%03u", checksum);
        command[N-3] = checksum_digits[0];
        command[N-2] = checksum_digits[1];
        command[N-1] = checksum_digits[2];
    }
}

bool is_command_uppercase(char *command) {
    unsigned int i;
    unsigned int N = strlen(command);

    for (i = 0; i < N; i++) {
        if (!(isupper(command[i]))) {
            return (false);
        }
    }

    return (true);
}

void wait_vacuum_not_busy() {
    if (is_vac_busy) {
        return;
    }

    send_command_to_vacuum(command);
    
    poll_recv_from_rimbox = wait_for_vacuum_response;
}

void send_command_to_vacuum(char *command) {
    is_vac_busy = true;

    strcpy (vac_request, command);
    vac_req_i = 0;

    is_vac_response_ready = false;

    vac_rspns_i = 0;

    start_timer(&vac_rspns_timeout_timer, vac_response_timeout, 700);

    poll_vac_session = vac_poll_send_request;
}

void wait_for_vacuum_response() {
    if (!(is_vac_response_ready)) {
        return;
    }

    if (is_parse_vacuum_response) {
        parse_vacuum_response(vac_response);
        is_parse_vacuum_response = false;
    }

    send_to_rimbox(vac_response);
    send_to_rimbox(EOL);

    free_vacuum_session();

    poll_recv_from_rimbox = purge_chars_from_rimbox;
}

void free_vacuum_session() {

    vac_req_i = 0;

    is_vac_busy = false;

    vac_rspns_i = 0;

    is_vac_response_ready = false;
}

void parse_vacuum_response(char *vacuum_response) {
    char len_str[3];
    int data_len;

    unsigned int rspns_len = strlen(vacuum_response);

    if (rspns_len > 10) {
        len_str[0] = vacuum_response[8];
        len_str[1] = vacuum_response[9];
        len_str[2] = 0;
        data_len = atoi(len_str);
        if ((data_len >= 2) && (data_len <= 16)) {
            if (rspns_len > (10 + data_len)) {
                memcpy(&vacuum_response[0], &vacuum_response[10], data_len);
                vacuum_response[data_len] = 0;
            }
        }
    }
}

void wait_cryo_not_busy() {
    unsigned int i;
    
    if (is_cryo_busy) {
        return;
    }

    send_command_to_cryo(command, args);

    poll_recv_from_rimbox = wait_for_cryo_response;
}

void send_command_to_cryo(char *command, char *args[]) {
    unsigned int i;
    
    is_cryo_busy = true;

    strcpy(cryo_request, command);

    if (args != NULL) {
        i = 0;
        while (!(args[i] == NULL)) {
            strcat(cryo_request, " ");
            strcat(cryo_request, args[i]);
            i = i + 1;
        }
    }

    cryo_req_i = 0;

    is_cryo_response_ready = false;

    cryo_rspns_i = 0;

    start_timer(&cryo_rspns_timeout_timer, cryo_response_timeout, 700);

    poll_cryo_session = cryo_poll_send_request;
}

void wait_for_cryo_response() {
    char *rspnslns[99];
    
    unsigned int rspnsln_i = 0;

    if (!(is_cryo_response_ready)) {
        return;
    }

    rspnslns[rspnsln_i] = strtok(cryo_response, "\r");

    while (!(rspnslns[rspnsln_i] == NULL)) {
        rspnsln_i = rspnsln_i + 1;
        rspnslns[rspnsln_i] = strtok(NULL, "\r");
    }

    for (rspnsln_i = 0; rspnslns[rspnsln_i] != NULL; rspnsln_i++) {
        send_to_rimbox(rspnslns[rspnsln_i]);
        if (rspnslns[rspnsln_i + 1] != NULL) {
            send_to_rimbox(LINESEP);
        }
    }

    send_to_rimbox(EOL);

    free_cryo_session();

    poll_recv_from_rimbox = purge_chars_from_rimbox;
}

void free_cryo_session() {
    cryo_req_i = 0;

    is_cryo_busy = false;

    cryo_rspns_i = 0;

    is_cryo_response_ready = false;
}

void poll_send_to_rimbox() {
    uint8_t rimbox_send_char;

    if (rimbox_send_count > 0) {
        if (UARTTransmitterIsReady(UART1)) {
            rimbox_send_char = rimbox_send_fifo[rimbox_send_tail_i];
            rimbox_send_tail_i = rimbox_send_tail_i + 1;
            if (!(rimbox_send_tail_i < RIMBOX_SEND_FIFO_SIZE)) {
                rimbox_send_tail_i = 0;
            }
            rimbox_send_count = rimbox_send_count - 1;
            UARTSendDataByte(UART1, rimbox_send_char);
        }
    }
}

void send_to_rimbox(char *msg) {
    unsigned int i;
    unsigned int N;

    N = strlen(msg);
    for (i = 0; i < N; i++) {
        rimbox_send_fifo[rimbox_send_head_i] = msg[i];
        rimbox_send_head_i = rimbox_send_head_i + 1;
        if (!(rimbox_send_head_i < RIMBOX_SEND_FIFO_SIZE)) {
            rimbox_send_head_i = 0;
        }
        rimbox_send_count = rimbox_send_count + 1;
    }
}

void send_char_to_rimbox(char c) {
    char c_str[2];

    c_str[0] = c;
    c_str[1] = 0;

    send_to_rimbox(c_str);
}

void vac_poll_idle() {
    if (UARTReceivedDataIsAvailable(UART3)) {
        UARTGetDataByte(UART3);
    }
}

void vac_poll_send_request() {
    if (!(UARTTransmitterIsReady(UART3))) {
        return;
    }

    char vac_char = vac_request[vac_req_i++];

    if (vac_char == 0) {
        vac_char = 0x0d;
        poll_vac_session = vac_poll_get_response;
    }

    UARTSendDataByte(UART3, vac_char);
}

void vac_poll_get_response() {
    if (!(UARTReceivedDataIsAvailable(UART3))) {
        return;
    }

    char vac_char = UARTGetDataByte(UART3);

    if (vac_char == 0x0d) {
        vac_response[vac_rspns_i] = 0;
        is_vac_response_ready = true;
        poll_vac_session = vac_poll_idle;
        return;
    }

    if ((vac_char >= 0x20) && (vac_char <= 0x7e)) {
        if (vac_rspns_i < (MAX_RESPONSE_LEN - 1)) {
            vac_response[vac_rspns_i] = vac_char;
            vac_rspns_i++;
        }
    }
}

void vac_response_timeout() {
    strcpy(vac_response, TIMEOUT);

    is_vac_response_ready = true;

    poll_vac_session = vac_poll_idle;
}

void cryo_poll_idle() {
    if (UARTReceivedDataIsAvailable(UART2)) {
        UARTGetDataByte(UART2);
    }
}

void cryo_poll_send_request() {
    if (!(UARTTransmitterIsReady(UART2))) {
        return;
    }

    char cryo_char = cryo_request[cryo_req_i++];

    if (cryo_char == 0) {
        cryo_char = 0x0d;
        poll_cryo_session = cryo_poll_get_response_first_line;
    }

    UARTSendDataByte(UART2, cryo_char);
}

void cryo_poll_get_response_first_line() {
    if (!(UARTReceivedDataIsAvailable(UART2))) {
        return;
    }

    char cryo_char = UARTGetDataByte(UART2);

    if (((cryo_char >= 0x20) && (cryo_char <= 0x7e)) || (cryo_char == 0x0d)){
        if (cryo_rspns_i < (MAX_RESPONSE_LEN - 1)) {
            cryo_response[cryo_rspns_i] = cryo_char;
            cryo_rspns_i++;
        }
    }

    if (cryo_char == 0x0d) {
        stop_timer(&cryo_rspns_timeout_timer);
        start_timer(&cryo_rspns_eol_timer,
            cryo_response_eol_timeout, 200);
        poll_cryo_session = cryo_poll_get_response_remaining_lines;
        return;
    }
}

void cryo_poll_get_response_remaining_lines() {
    if (!(UARTReceivedDataIsAvailable(UART2))) {
        return;
    }

    char cryo_char = UARTGetDataByte(UART2);

    if (((cryo_char >= 0x20) && (cryo_char <= 0x7e)) || (cryo_char == 0x0d)){
        if (cryo_rspns_i < (MAX_RESPONSE_LEN - 1)) {
            cryo_response[cryo_rspns_i] = cryo_char;
            cryo_rspns_i++;
        }
    }
}

void cryo_response_timeout() {
    strcpy(cryo_response, TIMEOUT);

    is_cryo_response_ready = true;

    poll_cryo_session = cryo_poll_idle;
}

void cryo_response_eol_timeout() {
    cryo_response[cryo_rspns_i] = 0;

    is_cryo_response_ready = true;

    poll_cryo_session = cryo_poll_idle;
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

/*
 * load adjustable parameters from INIT.TXT in the flash filesystem
 */
void load_init() {

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

void load_relay_state() {
    FIL fp;
    FRESULT rslt;
    UINT bytes_read;

    char text_state[19];

    relay_state = false;

    rslt = f_open(&fp, "RLYSTATE.TXT", (FA_READ));
    if (rslt == FR_OK) {
        f_read(&fp, text_state, 3, &bytes_read);
        f_close(&fp);
        if (bytes_read >= 2) {
            text_state[bytes_read] = 0;
            if (strcasecmp(text_state, "on") == 0) {
                mPORTGSetBits(BIT_0);
                relay_state = true;
            }
        }
    }
}

void save_relay_state() {
    FIL fp;
    FRESULT rslt;
    UINT bytes_written;

    char text_state[9];

    strcpy(text_state, (relay_state == true ? "on" : "off"));

    rslt = f_open(&fp, "RLYSTATE.TXT", (FA_CREATE_ALWAYS | FA_WRITE));
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

/*
 * open LOG.TXT in the flash filesystem and truncate it to zero length
 */
void init_log() {
    FIL fp;
    FRESULT rslt;

    rslt = f_open(&fp, "LOG.TXT", (FA_CREATE_ALWAYS | FA_WRITE));
    if (rslt == FR_OK) {
        f_close(&fp);
    }
}

/*
 * append a message to LOG.TXT in the flash filesystem
 */
void feedlog(char *msg) {
    FIL fp;
    FRESULT rslt;
    UINT bytes_written;

    char msgwithlf[99];

    strcpy (msgwithlf, msg);
    strcat (msgwithlf, "\n");

    rslt = f_open(&fp, "LOG.TXT", (FA_OPEN_ALWAYS | FA_WRITE));
    if (rslt == FR_OK) {
        rslt = f_lseek(&fp, f_size(&fp));
        if (rslt == FR_OK) {
            f_write(&fp, msgwithlf, strlen(msgwithlf), &bytes_written);
        }
        f_close(&fp);
    }
}

#ifdef MYDEBUG
void mylog (char *msg) {
    send_to_rimbox(msg);
    send_to_rimbox(EOL);
}
#endif

// command handler functions ..........................................

void help_command(char *args[]) {
    char pathfilename[99];
    char *help_args[1];

    unsigned int cmnds_i;

    help_args[0] = pathfilename;

    if (args[0] == NULL) {
        strcpy(pathfilename, "help.txt");
        cat_command(help_args);
        return;
    }

    if (strcasecmp(args[0], "commands") == 0) {
        strcpy(pathfilename, "help/commands.txt");
        cat_command(help_args);
        return;
    }

    for (cmnds_i = 0; cmnds_i < NUMBER_OF_COMMANDS; cmnds_i++) {
        if (strcasecmp(args[0], commands[cmnds_i].name) == 0) {
            strcpy(pathfilename, "help/");
            strcat(pathfilename, commands[cmnds_i].helpfile);
            cat_command(help_args);
            return;
        }
    }

    send_to_rimbox(EOL);
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

void gettemp_command(char *args[]) {
    char msg[99];
    unsigned int i;
    unsigned int nmbr_of_devices;
    struct tc74 *tc74_devices;
    float temp;
    bool is_fahrenheit = false;

    if (args[0] == NULL) {
        send_to_rimbox(EOL);
        return;
    }

//    N = strlen(args[0]);
//    for (i = 0; i < N; i++) {
//        args[0][i] = (char) toupper(args[0][i]);
//    }

    if (args[1] != NULL) {
        if (strcmp(args[1], "-f") == 0) {
            is_fahrenheit = true;
        }
        else {
            send_to_rimbox(EOL);
            return;
        }
    }

    nmbr_of_devices = gettc74table(&tc74_devices);

    for (i = 0; i < nmbr_of_devices; i++) {
        if (strcasecmp(args[0], tc74_devices[i].name) == 0) {
            if (!(tc74_devices[i].is_initialized)) {
                send_to_rimbox("not initialized");
                send_to_rimbox(EOL);
                return;
            }
            if (!(tc74_devices[i].is_valid_temp)) {
                send_to_rimbox("not valid");
                send_to_rimbox(EOL);
                return;
            }
            temp = tc74_devices[i].temp_c;
            if (is_fahrenheit) {
                temp = ((9.0 / 5.0) * temp) + 32.0;
            }
            sprintf(msg, "%3.1f", temp);
            send_to_rimbox(msg);
            send_to_rimbox(EOL);
            return;
        }
    }

    send_to_rimbox(EOL);
}

void getvacuum_command(char *args[]) {
    char msg[19];
    unsigned int i;

    float adc4_value;
    float U;
    float pTorr;
    float pmbar;

    float dTorr = 11.46;
    float dmbar = 11.33;

    struct adc *adc_inputs;

    unsigned int nmbr_of_adc_inputs = getadctable(&adc_inputs);

    for (i = 0; i < nmbr_of_adc_inputs; i++) {
        if (adc_inputs[i].address == 4) {
            if (!(adc_inputs[i].is_valid_value)) {
                send_to_rimbox(EOL);
                return;
            }
            adc4_value = adc_inputs[i].value;
        }
    }

    /*
     * 4.0 term is 3.0K over 1.0K input resistor network
     * 3.0 term is VRef+ precision voltage reference
     * 1024.0 term is 10-bit ADC
     */
    U = 4.0 * (3.0 * (adc4_value / 1024.0));

    if (!(args[0] == NULL)) {
        if (strcmp(args[0], "-v") == 0) {
            sprintf(msg, "%04.2f V", U);
            send_to_rimbox(msg);
            send_to_rimbox(EOL);
            return;
        }
        else {
            send_to_rimbox(EOL);
            return;
        }
    }

    if (U < 0.5) {
        send_to_rimbox("sensor error");
        send_to_rimbox(EOL);
        return;
    }

    if (U < 1.8) {
        send_to_rimbox("underrange");
        send_to_rimbox(EOL);
        return;
    }

    if (U >= 9.5) {
        send_to_rimbox("sensor error");
        send_to_rimbox(EOL);
        return;
    }

    if (U > 8.6) {
        send_to_rimbox("overrange");
        send_to_rimbox(EOL);
        return;
    }

    // Matt asked to switch to mbar instead of Torr
//    pTorr = pow(10.0, ((1.667 * U) - dTorr));
//    sprintf(msg, "%.1e Torr", pTorr);
    pmbar = pow(10.0, ((1.667 * U) - dmbar));
    sprintf(msg, "%3.1e", pmbar);
    send_to_rimbox(msg);
    send_to_rimbox(EOL);
}

void getdiode_command(char *args[]) {
    char msg[19];
    unsigned int i;

    float adc3_value;
    float diode_voltage;
    float tempk;
    float m;
    float b;

    struct adc *adc_inputs;

    unsigned int nmbr_of_adc_inputs = getadctable(&adc_inputs);

    // it is assumed there is an address 3 in the table
    for (i = 0; i < nmbr_of_adc_inputs; i++) {
        if (adc_inputs[i].address == 3) {
            if (!(adc_inputs[i].is_valid_value)) {
                send_to_rimbox(EOL);
                return;
            }
            adc3_value = adc_inputs[i].value;
        }
    }

    /*
     * 3.0 term is VRef+ precision voltage reference
     * 1024.0 term is 10-bit ADC
     */
    diode_voltage = 3.0 * (adc3_value / 1024.0);

    if (!(args[0] == NULL)) {
        if (strcmp(args[0], "-v") == 0) {
            sprintf(msg, "%05.3f", diode_voltage);
            send_to_rimbox(msg);
            send_to_rimbox(EOL);
            return;
        }
        else {
            send_to_rimbox(EOL);
            return;
        }
    }

    if (diode_voltage > diode_table[0].voltage) {
        send_to_rimbox("overvoltage");
        send_to_rimbox(EOL);
        return;
    }

    for (i = 1; i < diode_N; i++) {
        if (diode_voltage >= diode_table[i].voltage) {
            break;
        }
    }

    if (!(i < diode_N)) {
        send_to_rimbox("undervoltage");
        send_to_rimbox(EOL);
        return;
    }

    // interpolate with line equation is good enough
    m = (diode_table[i].tempK - diode_table[i-1].tempK) /
            (diode_table[i].voltage - diode_table[i-1].voltage);
    b = diode_table[i].tempK - (m * diode_table[i].voltage);
    
    tempk = (m * diode_voltage) + b;

    sprintf(msg, "%03.1f", tempk);
    send_to_rimbox(msg);
    send_to_rimbox(EOL);   
}

void getaccel_command(char *args[]) {
    get_accel_process(args);
}

void setrelay_command(char *args[]) {

    if (args[0] == NULL) {
        send_to_rimbox(EOL);
        return;
    }

    if (strcasecmp(args[0], "off") == 0) {
        mPORTGClearBits(BIT_0);
        relay_state = false;
        save_relay_state();
        send_to_rimbox(OK);
        send_to_rimbox(EOL);
        return;
    }

    if (strcasecmp(args[0], "on") == 0) {
        mPORTGSetBits(BIT_0);
        relay_state = true;
        save_relay_state();
        send_to_rimbox(OK);
        send_to_rimbox(EOL);
        return;
    }
 
    // command parameter not "off" or "on"
    send_to_rimbox(EOL);
    return;
}

void getrelay_command(char *args[]) {
    char msg[19];

    if (relay_state == true) {
        strcpy(msg, "on");
    }
    else {
        strcpy(msg, "off");
    }

    send_to_rimbox(msg);
    send_to_rimbox(EOL);
}

/*
 * list files in directory of flash filesystem
 */
void ls_command(char *args[]) {
    char msg[99];

    FRESULT res;
    FILINFO fno;
    DIR dir;
    int i;

    res = f_opendir(&dir, (args[0] == NULL ? "" : args[0]));
    if (res == FR_OK) {
        for (i = 0;; i++) {
            res = f_readdir(&dir, &fno);
            if ((res != FR_OK) || (fno.fname[0] == 0)) {
                break;
            }
            if (i > 0) {
                send_to_rimbox(LINESEP);
            }
            if (fno.fattrib & AM_DIR) {
                sprintf(msg, "%-12s  %7s", fno.fname, "<DIR>");
            }
            else {
                sprintf(msg, "%-12s  %7u", fno.fname, fno.fsize);
            }
            send_to_rimbox(msg);
        }
    }

    send_to_rimbox(EOL);
}

/*
 * display contents of .DAT and .TXT files stored in flash filesystem.
 * Files with other extensions are excluded because they might be
 * large and/or binary.
 */
void cat_command(char *args[]) {
    unsigned int i;
    char cat_char;

#define CAT_READ_BFR_LEN 80
    char read_bfr[CAT_READ_BFR_LEN];

    FIL fp;

    FRESULT res;
    UINT read_count;

    char *pext;

    if (args[0] == NULL) {
        send_to_rimbox(EOL);
        return;
    }

    pext = strrchr(args[0], '.');

    if (pext == NULL) {
        send_to_rimbox(EOL);
        return;
    }

    if ((strcasecmp(pext, ".dat") != 0) && (strcasecmp(pext, ".txt") != 0)) {
        send_to_rimbox(EOL);
        return;
    }

    res = f_open(&fp, args[0], FA_READ);
    if (res == FR_OK) {
        do {
            res = f_read(&fp, read_bfr, (UINT) CAT_READ_BFR_LEN, &read_count);
            if (!(res == FR_OK)) {
                break;
            }
            if (read_count > 0) {
                for (i = 0; i < read_count; i++) {
                    cat_char = read_bfr[i];
                    if (cat_char == 0x0a) {
                        send_to_rimbox(LINESEP);
                    }
                    if ((cat_char >= 0x20) && (cat_char <= 0x7e)) {
                        send_char_to_rimbox(cat_char);
                    }
                }
            }
        }
        while (read_count == CAT_READ_BFR_LEN);
    }

    send_to_rimbox(EOL);
}

void get24v_command(char *args[]) {
    char msg[19];

    unsigned int i;

    float adc2_value;
    float v24;

    struct adc *adc_inputs;

    unsigned int nmbr_of_adc_inputs = getadctable(&adc_inputs);

    // it is assumed there is an address 2 in the table
    for (i = 0; i < nmbr_of_adc_inputs; i++) {
        if (adc_inputs[i].address == 2) {
            if (!(adc_inputs[i].is_valid_value)) {
                send_to_rimbox(EOL);
                return;
            }
            adc2_value = adc_inputs[i].value;
        }
    }

    /*
     * 3.0 term is VRef+ precision voltage reference
     * 1024.0 term is 10-bit ADC
     */
    v24 = ((23.7 + 1.0) / 1.0) * (3.0 * (adc2_value / 1024.0));


    sprintf(msg, "%03.1f", v24);
    send_to_rimbox(msg);
    send_to_rimbox(EOL);
}

void get48v_command(char *args[]) {
    char msg[19];

    unsigned int i;

    float adc5_value;
    float v48;

    struct adc *adc_inputs;

    unsigned int nmbr_of_adc_inputs = getadctable(&adc_inputs);

    // it is assumed there is an address 5 in the table
    for (i = 0; i < nmbr_of_adc_inputs; i++) {
        if (adc_inputs[i].address == 5) {
            if (!(adc_inputs[i].is_valid_value)) {
                send_to_rimbox(EOL);
                return;
            }
            adc5_value = adc_inputs[i].value;
        }
    }

    /*
     * 3.0 term is VRef+ precision voltage reference
     * 1024.0 term is 10-bit ADC
     */
    v48 = ((47.5 + 1.0) / 1.0) * (3.0 * (adc5_value / 1024.0));


    sprintf(msg, "%03.1f", v48);
    send_to_rimbox(msg);
    send_to_rimbox(EOL);
}

void getcryoattemp_command(char *args[]) {
    char msg[19];

    if (mPORTFReadBits(BIT_13)) {
        strcpy(msg, "yes");
    }
    else {
        strcpy(msg, "no");
    }

    send_to_rimbox(msg);
    send_to_rimbox(EOL);
}

void setfeedstartmode_command(char *args[]) {
    if (args[0] == NULL) {
        send_to_rimbox(EOL);
        return;
    }

    if (strcasecmp(args[0], "manual") == 0) {
        auto_start_state = false;
        save_autostart_state();
        send_to_rimbox(OK);
        send_to_rimbox(EOL);
        return;
    }

    if (strcasecmp(args[0], "auto") == 0) {
        auto_start_state = true;
        save_autostart_state();
//      poll_auto_start = auto_start_idle;
        send_to_rimbox(OK);
        send_to_rimbox(EOL);
        return;
    }

    // command parameter not "manual" or "auto"
    send_to_rimbox(EOL);
    return;
}

void getfeedstartmode_command(char *args[]) {
    char msg[19];

    if (auto_start_state == true) {
        strcpy(msg, "auto");
    }
    else {
        strcpy(msg, "manual");
    }

    send_to_rimbox(msg);
    send_to_rimbox(EOL);
}

/*
 * this puts the control board in a mode best for interaction with
 * a terminal (such as at Minex)
 */
void hyperterminal_command(char *args[]) {
    echo_mode = true;

    EOL = CRLF;
    LINESEP = EOL;

    send_to_rimbox(OK);
    send_to_rimbox(EOL);
}

/*
 * this puts the control board in a mode suitable for interaction
 * through the rimbox controller virtual TCP/IP to RS-232 port.  Most
 * important is the use of '|' as a line separator.  The virtual link
 * only supports return of a single line so multiple line responses
 * are separated by the '|' character.
 */
void stty_command(char *args[]) {
    echo_mode = false;

    EOL = CR;
    LINESEP = VBAR;

    send_to_rimbox(OK);
    send_to_rimbox(EOL);
}

void getversion_command(char *args[]) {
    char msg[19];

    sprintf(msg, "%4.2f", version);

    send_to_rimbox(msg);
    send_to_rimbox(EOL);
}

void bootloader_command(char *args[]) {
    SoftReset();
}

/*
 * reset the microcontroller board by triggering the watchdog
 */
void reset_command(char *args[]) {
    send_to_rimbox("reset in one minute...");
    send_to_rimbox(EOL);
    while (true) {
        poll_send_to_rimbox();
    };
}

// TODO: embed scripting engine for auto start procedure

void auto_start_idle() {
    if (auto_start_state == true) {
        start_timer(&auto_start_timer, auto_start_timer_callback,
                AUTO_START_1_MIN);
        auto_start_next_state = auto_start_000_p009_request;
        poll_auto_start = auto_start_delay;
    }
}

void auto_start_delay() {

}

void auto_start_timer_callback() {
    poll_auto_start = auto_start_next_state;
}

void auto_start_000_p009_request() {
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

}

void auto_start_complete() {
    
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
