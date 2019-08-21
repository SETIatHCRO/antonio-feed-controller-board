#include <plib.h>           /* Include to use PIC32 peripheral libraries      */
#include <stdio.h>
#include <stdint.h>         /* For uint32_t definition                        */
#include <stdbool.h>        /* For true/false definition     */
#include <stdlib.h>
#include <math.h>

#include "oneshot.h"
#include "accel_1.h"

extern char *LINESEP;
extern char *EOL;

#ifdef MYACCELDEBUG
static char accel_msg[99];
#endif

static uint8_t accel_address = 0x1d;
static bool is_accel_initialized = false;
static bool is_accel_timeout;

#define SMPLS_PER_SEC 400

static float a = 1.0 / (float) SMPLS_PER_SEC;

static float accel_xmin, accel_xmean, accel_xvar, accel_xmax;
static float accel_ymin, accel_ymean, accel_yvar, accel_ymax;
static float accel_zmin, accel_zmean, accel_zvar, accel_zmax;

static unsigned int smpli = 0;

static float xsmpls[SMPLS_PER_SEC];
static float ysmpls[SMPLS_PER_SEC];
static float zsmpls[SMPLS_PER_SEC];

static unsigned int aregi = 0;

static bool is_data_read;
static uint8_t accel_data;

static struct accel_reg_op accel_reg[] = {
    {0x0d, 1,    0, false},
    {0x2b, 0, 0x40, false},  // software reset
    {0x2b, 1,    0, false},  // read CTRL_REG2 reset status
//  {0x0e, 0, 0x11, false},  // high-pass filter, +/- 4g range
    {0x0e, 0, 0x01, false},  // no filter, +/- 4g range
    {0x0f, 0, 0x03, false},  // high-pass filter 2Hz cut-off @ 400Hz
    {0x2c, 0, 0x00, false},  // interrupt active low, push-pull
    {0x2d, 0, 0x01, false},  // data ready interrupt enables
    {0x2e, 0, 0x01, false},  // data ready interrupt on INT 1 pin
//  {0x2a, 0, 0x39, false},  // 1.56 Hz data rate, set active
    {0x2a, 0, 0x09, false},  // 400 Hz data rate, set active
    {0x00, 1,    0, false},  // data status
    {0x01, 1,    0, true},   // MSB X
    {0x02, 1,    0, true},   // LSB X
    {0x03, 1,    0, true},   // MSB Y
    {0x04, 1,    0, true},   // LSB Y
    {0x05, 1,    0, true},   // MSB Z
    {0x06, 1,    0, false}   // LSB Z
};

#define NMBR_OF_ACCEL_REGS (sizeof (accel_reg) / sizeof(struct accel_reg_op))

static struct oneshot_timer accel_timeout_timer = {"accel_timeout_timer", 0, NULL, NULL};

static void (*volatile accel_state)();
static void (*accel_timeout_state)();

bool one_second_report = false;
static struct oneshot_timer accel_report_timer = {"accel_report_timer", 0, NULL, NULL};
static void accel_one_sec_report_callback();
static void accel_report_one_sec(bool on_or_off);

void getaccel_command(char *args[]) {
    get_accel_process(args);
}

void init_accel() {
#ifdef MYACCELDEBUG
//  mylog("init_accel");
#endif
    accel_state = accel_start;

    is_accel_timeout = false;
    accel_timeout_state = accel_stop;
    start_timer(&accel_timeout_timer, accel_timeout_callback, 100);

    aregi = 0;

    accel_xmin = 99.0;
    accel_ymin = 99.0;
    accel_zmin = 99.0;

    accel_xmean = 0.0;
    accel_ymean = 0.0;
    accel_zmean = 0.0;

    accel_xvar = 0.0;
    accel_yvar = 0.0;
    accel_zvar = 0.0;

    accel_xmax = -99.0;
    accel_ymax = -99.0;
    accel_zmax = -99.0;
}

void get_accel_process(char *args[]) {
    char msg[99];

    if (args != NULL && args[0] != NULL) {
        get_accel_process_raw(args);
        return;
    }

    sprintf(msg, "%6.3f  %6.3f  %6.3f  %6.3f", accel_xmin, accel_xmean, sqrt(accel_xvar), accel_xmax);
    send_to_rimbox(msg);
    send_to_rimbox(LINESEP);
    sprintf(msg, "%6.3f  %6.3f  %6.3f  %6.3f", accel_ymin, accel_ymean, sqrt(accel_yvar), accel_ymax);
    send_to_rimbox(msg);
    send_to_rimbox(LINESEP);
    sprintf(msg, "%6.3f  %6.3f  %6.3f  %6.3f", accel_zmin, accel_zmean, sqrt(accel_zvar), accel_zmax);
    send_to_rimbox(msg);
    send_to_rimbox(EOL);
    
    accel_xmin = 99.0;
    accel_ymin = 99.0;
    accel_zmin = 99.0;

    accel_xmax = -99.0;
    accel_ymax = -99.0;
    accel_zmax = -99.0;
}

void get_accel_process_raw(char *args[]) {
    char sbfr[19];

    unsigned int N = SMPLS_PER_SEC;

    unsigned int i;
    unsigned int local_smpli = smpli;
    float *smpls = NULL;

    if (strcasecmp(args[0], "-d") != 0) {
        send_to_rimbox(EOL);
        return;
    }

    if (args[1] == NULL) {
        send_to_rimbox(EOL);
        return;
    }

    if (strcasecmp(args[1], "x") == 0) {
        smpls = xsmpls;
    }
    if (strcasecmp(args[1], "y") == 0) {
        smpls = ysmpls;
    }
    if (strcasecmp(args[1], "z") == 0) {
        smpls = zsmpls;
    }

    if (smpls == NULL) {
        send_to_rimbox(EOL);
        return;
    }

    if (args[2] != NULL) {
        N = (unsigned int) atoi(args[2]);
        if (!((N > 0) && (N <= SMPLS_PER_SEC))) {
            send_to_rimbox(EOL);
            return;
        }
    }

    local_smpli++;
    for (i = 0; i < N; i++) {
        if (!(local_smpli < N)) {
            local_smpli = 0;
        }
        if (i > 0) {
            send_to_rimbox(",");
        }
        sprintf(sbfr, "%5.3f", smpls[local_smpli]);
        send_to_rimbox(sbfr);
        local_smpli++;
    }

    send_to_rimbox(EOL);
}

static void accel_timeout_callback() {
#ifdef MYACCELDEBUG
//  mylog("accel_timeout_callback");
#endif
#ifdef MYACCELDEBUG
    strcat(accel_msg, " TIMEOUT");
#endif
    
    is_accel_timeout = true;

    accel_state = accel_timeout_state;
    
}

void poll_accel() {
    accel_state();
}

static void accel_idle() {
    if (is_accel_initialized == false) {
        return;
    }

    // poll the INT 1 pin for data ready
    if (!(PORTReadBits(IOPORT_E, BIT_8))) {
        accel_state = accel_start;
        is_accel_timeout = false;
        accel_timeout_state = accel_stop;
        start_timer(&accel_timeout_timer, accel_timeout_callback, 100);
        aregi = 9;
    }
}

static void accel_start() {
#ifdef MYACCELDEBUG
    char msg[19];
#endif
#ifdef MYACCELDEBUG
//  mylog("accel_start");
#endif
    I2C_RESULT i2c_result;

    if (I2CBusIsIdle(I2C2)) {
        
#ifdef MYACCELDEBUG
        strcpy(accel_msg, "START");
#endif
        is_data_read = false;
        i2c_result = I2CStart(I2C2);
        if (i2c_result == I2C_SUCCESS) {
            accel_state = accel_wait;
        }
        else {
#ifdef MYACCELDEBUG
            sprintf(msg, " i2c_result %d", i2c_result);
            strcat(accel_msg, msg);
#endif
            accel_state = accel_stop;
        }
    }
}

static void accel_wait() {
    if (I2CGetStatus(I2C2) & I2C_START) {
        accel_state = accel_send_addr;
    }
}

static void accel_send_addr() {
#ifdef MYACCELDEBUG
    char msg[19];
#endif
    I2C_RESULT i2c_result;

    uint8_t addr = (accel_address << 1) | 0x00;

    if (I2CTransmitterIsReady(I2C2)) {
#ifdef MYACCELDEBUG
        sprintf(msg, " 0x%02x", addr);
        strcat(accel_msg, msg);
#endif
        i2c_result = I2CSendByte(I2C2, addr);
        accel_state = accel_send_addr_wait;
    }
}

static void accel_send_addr_wait() {
    if (I2CTransmissionHasCompleted(I2C2)) {
        if (I2CByteWasAcknowledged(I2C2)) {
#ifdef MYACCELDEBUG
            strcat(accel_msg, " ACK");
#endif
            accel_state = accel_send_reg;
        }
        else {
#ifdef MYACCELDEBUG
            strcat(accel_msg, " NACK");
#endif
            accel_state = accel_stop;
        }
    }
}

static void accel_send_reg() {
#ifdef MYACCELDEBUG
    char msg[19];
#endif
    I2C_RESULT i2c_result;

    uint8_t reg_num = accel_reg[aregi].num;

    if (I2CTransmitterIsReady(I2C2)) {
#ifdef MYACCELDEBUG
        sprintf(msg, " 0x%02x", reg_num);
        strcat(accel_msg, msg);
#endif
        i2c_result = I2CSendByte(I2C2, reg_num);
        accel_state = accel_send_reg_wait;
    }
}

static void accel_send_reg_wait() {
    if (I2CTransmissionHasCompleted(I2C2)) {
        if (I2CByteWasAcknowledged(I2C2)) {
#ifdef MYACCELDEBUG
            strcat(accel_msg, " ACK");
#endif
            if (accel_reg[aregi].rw == 0) {
                accel_state = accel_write_data;
                return;
            }
            if (accel_reg[aregi].rw == 1) {
                accel_state = accel_repeat_start;
                return;
            }
        }
        else {
#ifdef MYACCELDEBUG
            strcat(accel_msg, " NACK");
#endif
        }
        accel_state = accel_stop;
    }
}

static void accel_write_data() {
#ifdef MYACCELDEBUG
    char msg[19];
#endif
    I2C_RESULT i2c_result;

    uint8_t value = accel_reg[aregi].val;

    if (I2CTransmitterIsReady(I2C2)) {
#ifdef MYACCELDEBUG
        sprintf(msg, " 0x%02x", value);
        strcat(accel_msg, msg);
#endif
        i2c_result = I2CSendByte(I2C2, value);
        accel_state = accel_write_data_wait;
    }
}

static void accel_write_data_wait() {
    if (I2CTransmissionHasCompleted(I2C2)) {
        if (I2CByteWasAcknowledged(I2C2)) {
#ifdef MYACCELDEBUG
            strcat(accel_msg, " ACK");
#endif
        }
        else {
#ifdef MYACCELDEBUG
            strcat(accel_msg, " NACK");
#endif
        }
        accel_state = accel_stop;
    }
}

static void accel_repeat_start() {
#ifdef MYACCELDEBUG
    char msg[19];
#endif
    I2C_RESULT i2c_result;

#ifdef MYACCELDEBUG
        strcat(accel_msg, " RE-START");
#endif
        i2c_result = I2CRepeatStart(I2C2);
        if (i2c_result == I2C_SUCCESS) {
            accel_state = accel_repeat_start_wait;
        }
        else {
#ifdef MYACCELDEBUG
//          sprintf(msg, " i2c_result %d", i2c_result);
//          strcat(accel_msg, msg);
#endif
            accel_state = accel_repeat_start_wait;
        }
}

static void accel_repeat_start_wait() {
    if (I2CGetStatus(I2C2) & I2C_START) {
        accel_state = accel_resend_addr;
    }
}

static void accel_resend_addr() {
#ifdef MYACCELDEBUG
    char msg[19];
#endif
    I2C_RESULT i2c_result;

    uint8_t addr = (accel_address << 1) | 0x01;

    if (I2CTransmitterIsReady(I2C2)) {
#ifdef MYACCELDEBUG
        sprintf(msg, " 0x%02x", addr);
        strcat(accel_msg, msg);
#endif
        i2c_result = I2CSendByte(I2C2, addr);
        accel_state = accel_resend_addr_wait;
    }
}

static void accel_resend_addr_wait() {
    if (I2CTransmissionHasCompleted(I2C2)) {
        if (I2CByteWasAcknowledged(I2C2)) {
#ifdef MYACCELDEBUG
            strcat(accel_msg, " ACK");
#endif
            I2CReceiverEnable(I2C2, TRUE);
            accel_state = accel_read_data;
        }
        else {
#ifdef MYACCELDEBUG
            strcat(accel_msg, " NACK");
#endif
            accel_state = accel_stop;
        }
    }
}

static void accel_read_data() {
#ifdef MYACCELDEBUG
    char msg[19];
#endif
    uint8_t value;

    if (I2CReceivedDataIsAvailable(I2C2)) {
        value = I2CGetByte(I2C2);
        is_data_read = true;
        accel_data = value;
#ifdef MYACCELDEBUG
        sprintf(msg, " 0x%02x", value);
        strcat(accel_msg, msg);
#endif
        if (accel_reg[aregi].is_chained) {
            accel_state = accel_send_ack;
        }
        else {
            accel_state = accel_send_nack;
        }
    }
}

static void accel_send_ack() {
#ifdef MYACCELDEBUG
    strcat(accel_msg, " ACK");
#endif
    I2CAcknowledgeByte(I2C2, TRUE);

    accel_state = accel_send_ack_wait;
}

static void accel_send_ack_wait() {
    if (I2CAcknowledgeHasCompleted(I2C2)) {
        if (is_accel_initialized) {
            accel_accumulate_stats();
        }
        aregi = aregi + 1;
        I2CReceiverEnable(I2C2, TRUE);
        accel_state = accel_read_data;
    }
}

static void accel_send_nack() {
#ifdef MYACCELDEBUG
    strcat(accel_msg, " NACK");
#endif
    I2CAcknowledgeByte(I2C2, FALSE);

    accel_state = accel_send_nack_wait;
}

static void accel_send_nack_wait() {
    if (I2CAcknowledgeHasCompleted(I2C2)) {
        accel_state = accel_stop;
    }
}

static void accel_stop() {
#ifdef MYACCELDEBUG
//  mylog("accel_stop");
#endif
    accel_timeout_state = accel_loop;
    start_timer(&accel_timeout_timer, accel_timeout_callback, 100);
#ifdef MYACCELDEBUG
    strcat(accel_msg, " STOP");
#endif
    //send an I2C Stop condition to terminate a transfer
    I2CStop(I2C2);

    accel_state = accel_stop_wait;
}

static void accel_stop_wait() {
    if (I2CGetStatus(I2C2) & I2C_STOP) {
        accel_state = accel_loop;
    }
}

static void accel_loop() {
    
    stop_timer(&accel_timeout_timer);
#ifdef MYACCELDEBUG
    mylog(accel_msg);
#endif

    if (is_accel_timeout) {
        accel_state = accel_idle;
        return;
    }

    // retry until software reset complete
    if ((accel_reg[aregi].num == 0x2b) && (accel_reg[aregi].rw == 1)) {
        if ((!(is_data_read)) || (is_data_read && (accel_data & 0x40))) {
            is_accel_timeout = false;
            accel_state = accel_start;
            accel_timeout_state = accel_stop;
            start_timer(&accel_timeout_timer, accel_timeout_callback, 100);
            return;
        }
    }

//    if ((accel_reg[aregi].num == 0x00) && is_data_read) {
//        if (accel_data & 0xf0) {
//            mylog("overrun");
//        }
//    }

    if (is_accel_initialized) {
        accel_accumulate_stats();
    }

    aregi = aregi + 1;

    if (aregi < NMBR_OF_ACCEL_REGS) {
        is_accel_timeout = false;
        accel_state = accel_start;
        accel_timeout_state = accel_stop;
        start_timer(&accel_timeout_timer, accel_timeout_callback, 100);
        return;
    }
    else {
        //JR debug
        //if(is_accel_initialized == false)
        //  accel_report_one_sec(true);
        is_accel_initialized = true;
        
    }

    accel_state = accel_idle;
}

static void accel_accumulate_stats() {
//  char msg[99];

    static int16_t accel_x_val;
    static int16_t accel_y_val;
    static int16_t accel_z_val;

    static float accel_x_g;
    static float accel_y_g;
    static float accel_z_g;

    if (accel_reg[aregi].num == 0x01) {
        accel_x_val = accel_data << 8;
    }

    if (accel_reg[aregi].num == 0x02) {
        accel_x_val |= (((uint16_t) accel_data) & 0x00ff);
        accel_x_g = ((float) (accel_x_val / 16)) * 0.002;
        xsmpls[smpli] = accel_x_g;
//      accel_xmean = (a * accel_x_g) + ((1.0 - a) * accel_xmean);
        accel_xmean = accel_xmean + (a * (accel_x_g - accel_xmean));
        accel_xvar = (a * (accel_x_g - accel_xmean) * (accel_x_g - accel_xmean)) +
                ((1.0 - a) * accel_xvar);
        if (accel_x_g < accel_xmin) {
            accel_xmin = accel_x_g;
        }
        if (accel_x_g > accel_xmax) {
            accel_xmax = accel_x_g;
        }
    }

    if (accel_reg[aregi].num == 0x03) {
        accel_y_val = accel_data << 8;
    }

    if (accel_reg[aregi].num == 0x04) {
        accel_y_val |= (((uint16_t) accel_data) & 0x00ff);
        accel_y_g = ((float) (accel_y_val / 16)) * 0.002;
        ysmpls[smpli] = accel_y_g;
//      accel_ymean = (a * accel_y_g) + ((1.0 - a) * accel_ymean);
        accel_ymean = accel_ymean + (a * (accel_y_g - accel_ymean));
        accel_yvar = (a * (accel_y_g - accel_ymean) * (accel_y_g - accel_ymean)) +
                ((1.0 - a) * accel_yvar);
        if (accel_y_g < accel_ymin) {
            accel_ymin = accel_y_g;
        }
        if (accel_y_g > accel_ymax) {
            accel_ymax = accel_y_g;
        }
    }

    if (accel_reg[aregi].num == 0x05) {
        accel_z_val = accel_data << 8;
    }

    if (accel_reg[aregi].num == 0x06) {
        accel_z_val |= (((uint16_t) accel_data) & 0x00ff);
        accel_z_g = ((float) (accel_z_val / 16)) * 0.002;
#ifdef MYACCELDEBUG
//      sprintf(msg, "%04x, %04x, %04x", accel_x_val, accel_y_val, accel_z_val);
        sprintf(msg, "%6.3f, %6.3f, %6.3f", accel_x_g, accel_y_g, accel_z_g);
        mylog(msg);
#endif
        zsmpls[smpli] = accel_z_g;
//      accel_zmean = (a * accel_z_g) + ((1.0 - a) * accel_zmean);
        accel_zmean = accel_zmean + (a * (accel_z_g - accel_zmean));
        accel_zvar = (a * (accel_z_g - accel_zmean) * (accel_z_g - accel_zmean)) +
                ((1.0 - a) * accel_zvar);
        if (accel_z_g < accel_zmin) {
            accel_zmin = accel_z_g;
        }
        if (accel_z_g > accel_zmax) {
            accel_zmax = accel_z_g;
        }
        smpli = smpli + 1;
        if (!(smpli < SMPLS_PER_SEC)) {
//          sprintf(msg, "%6.3f, %6.3f, %6.3f", accel_x_g, accel_y_g, accel_z_g);
//          sprintf(msg, "%6.3f, %6.3f, %6.3f, %6.3f", accel_xmin, accel_xmean, accel_xvar, accel_xmax);
//          mylog(msg);
//          sprintf(msg, "%6.3f, %6.3f, %6.3f, %6.3f", accel_ymin, accel_ymean, accel_yvar, accel_ymax);
//          mylog(msg);
//          sprintf(msg, "%6.3f, %6.3f, %6.3f, %6.3f", accel_zmin, accel_zmean, accel_zvar, accel_zmax);
//          mylog(msg);
            smpli = 0;
        }
    }
}

void accelonesec_command(char *args[]) {

    if (args[0] == NULL) {
        //send_to_rimbox("Syntax: accelonesec on/off\r\n");
        if(one_second_report == true) {
            accel_report_one_sec(false);
        }
        else {
            accel_report_one_sec(true);
        }
        return;
    }

    if (strcasecmp(args[0], "off") == 0) {
        accel_report_one_sec(false);
    }
    if (strcasecmp(args[0], "on") == 0) {
        accel_report_one_sec(true);
    }
    
    
}

static void accel_report_one_sec(bool on_or_off) {
    
    one_second_report = on_or_off;
    accel_one_sec_report_callback();
}

static void accel_one_sec_report_callback() {
    
    if(!one_second_report) return;
    send_to_rimbox("\r\n");
    get_accel_process(NULL);
    start_timer(&accel_report_timer, accel_one_sec_report_callback, 1000);
}