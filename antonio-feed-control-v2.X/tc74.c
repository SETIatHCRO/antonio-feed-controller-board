#include <plib.h>           /* Include to use PIC32 peripheral libraries      */
#include <stdio.h>
#include <stdint.h>         /* For uint32_t definition                        */
#include <stdbool.h>        /* For true/false definition     */
#include <stdlib.h>
#include <math.h>

#include "oneshot.h"
#include "accel.h"
#include "tc74.h"

#include "rimbox.h"

extern char *EOL;

#ifdef MYTC74DEBUG
    char tc74_msg[99];
#endif

static unsigned int tc74_i = 0;

static struct tc74 tc74_devices[] = {
    {"A0", 0x90, false, false, 0.0},
    {"A1", 0x92, false, false, 0.0},
    {"A2", 0x94, false, false, 0.0},
    {"A3", 0x96, false, false, 0.0},
    {"A4", 0x98, false, false, 0.0},
    {"A5", 0x9a, false, false, 0.0},
    {"A6", 0x9c, false, false, 0.0},
    {"A7", 0x9e, false, false, 0.0}
};

#define NMBR_OF_TC74_DEVICES (sizeof (tc74_devices) / sizeof(struct tc74))

static struct oneshot_timer tc74_start_timer = {"tc74_start_timer", 0, NULL, NULL};
static struct oneshot_timer tc74_timeout_timer = {"tc74_timeout_timer", 0, NULL, NULL};

static void (*poll_tc74_devices_state)();
static void (*tc74_timeout_state)();

#ifdef MYTC74DEBUG
void dump_tc74_device_table();
#endif

void init_tc74() {
    poll_tc74_devices_state = tc74_devices_idle;

    start_timer(&tc74_start_timer, tc74_start_callback, 1000);
}

unsigned int gettc74table(struct tc74 **tc74tblptr) {
    *tc74tblptr = tc74_devices;

    return (NMBR_OF_TC74_DEVICES);
}

static void tc74_start_callback() {
    if (poll_tc74_devices_state == tc74_devices_idle) {
#ifdef MYTC74DEBUG
        strcpy(tc74_msg, "");
#endif
        tc74_timeout_state = tc74_devices_stop;
        start_timer(&tc74_timeout_timer, tc74_timeout_callback, 100);
        poll_tc74_devices_state = tc74_devices_start;
    }

    // sample temp. sensors every 1 second
    start_timer(&tc74_start_timer, tc74_start_callback, 1000);
}

static void tc74_timeout_callback() {
    send_to_rimbox("tc74 init error");
    send_to_rimbox(EOL);
    tc74_devices[tc74_i].is_initialized = false;
    tc74_devices[tc74_i].is_valid_temp = false;
    tc74_timeout_state();
}

void poll_tc74() {
    poll_tc74_devices_state();
}

static void tc74_devices_idle() {

}

static void tc74_devices_start() {
    I2C_RESULT i2c_result;

    if (I2CBusIsIdle(I2C1)) {
#ifdef MYTC74DEBUG
        strcat(tc74_msg, "START");
#endif
        i2c_result = I2CStart(I2C1);
        if (i2c_result == I2C_SUCCESS) {
            poll_tc74_devices_state = tc74_devices_start_wait;
        }
        else {
            tc74_devices[tc74_i].is_initialized = false;
            tc74_devices[tc74_i].is_valid_temp = false;
            poll_tc74_devices_state = tc74_devices_stop;
        }
    }
}

static void tc74_devices_start_wait() {
    if (I2CGetStatus(I2C1) & I2C_START) {
        poll_tc74_devices_state = tc74_devices_send_addr;
    }
}

static void tc74_devices_send_addr() {
#ifdef MYTC74DEBUG
    char msg[99];
#endif
    I2C_RESULT i2c_result;

    uint8_t addr = tc74_devices[tc74_i].address;

    if (tc74_devices[tc74_i].is_initialized) {
        addr = addr | 0x01;
    }

    if (I2CTransmitterIsReady(I2C1)) {
#ifdef MYTC74DEBUG
        sprintf(msg, " 0x%02x", addr);
        strcat(tc74_msg, msg);
#endif
        i2c_result = I2CSendByte(I2C1, addr);
        poll_tc74_devices_state = tc74_devices_send_addr_wait;
    }
}

static void tc74_devices_send_addr_wait() {
    if (I2CTransmissionHasCompleted(I2C1)) {
        if (I2CByteWasAcknowledged(I2C1)) {
#ifdef MYTC74DEBUG
            strcat(tc74_msg, " ACK");
#endif
            if (tc74_devices[tc74_i].is_initialized) {
                I2CReceiverEnable(I2C1, TRUE);
                poll_tc74_devices_state = tc74_devices_read_data;
            }
            else {
                poll_tc74_devices_state = tc74_devices_send_read_cmnd;
            }
        }
        else {
#ifdef MYTC74DEBUG
            strcat(tc74_msg, " NACK");
#endif
            tc74_devices[tc74_i].is_initialized = false;
            tc74_devices[tc74_i].is_valid_temp = false;
            poll_tc74_devices_state = tc74_devices_stop;
        }
    }
}

static void tc74_devices_send_read_cmnd() {
#ifdef MYTC74DEBUG
    char msg[99];
#endif
    I2C_RESULT i2c_result;

    uint8_t read_cmnd = 0x00;

    if (I2CTransmitterIsReady(I2C1)) {
#ifdef MYTC74DEBUG
        sprintf(msg, " 0x%02x", read_cmnd);
        strcat(tc74_msg, msg);
#endif
        i2c_result = I2CSendByte(I2C1, read_cmnd);
        poll_tc74_devices_state = tc74_devices_send_read_cmnd_wait;
    }
}

static void tc74_devices_send_read_cmnd_wait() {
    
    if (I2CTransmissionHasCompleted(I2C1)) {
        if (I2CByteWasAcknowledged(I2C1)) {
#ifdef MYTC74DEBUG
            strcat(tc74_msg, " ACK");
#endif
            tc74_devices[tc74_i].is_initialized = true;
        }
        else {
#ifdef MYTC74DEBUG
            strcat(tc74_msg, " NACK");
#endif
        }
        poll_tc74_devices_state = tc74_devices_stop;
    }
}

static void tc74_devices_read_data() {
#ifdef MYTC74DEBUG
    char msg[99];
#endif
    int8_t temp_c;

    if (I2CReceivedDataIsAvailable(I2C1)) {
        temp_c = I2CGetByte(I2C1);
#ifdef MYTC74DEBUG
        sprintf(msg, " 0x%02x", temp_c);
        strcat(tc74_msg, msg);
#endif
        if (!(tc74_devices[tc74_i].is_valid_temp)) {
            tc74_devices[tc74_i].temp_c = (float) temp_c;
            tc74_devices[tc74_i].is_valid_temp = true;
        }
        tc74_devices[tc74_i].temp_c =
                (0.9 * tc74_devices[tc74_i].temp_c) + (0.1 * (float) temp_c);
        poll_tc74_devices_state = tc74_devices_send_nack;
    }
}

static void tc74_devices_send_nack() {
#ifdef MYTC74DEBUG
    strcat(tc74_msg, " NACK");
#endif
    I2CAcknowledgeByte(I2C1, FALSE);
    poll_tc74_devices_state = tc74_devices_send_nack_wait;
}

static void tc74_devices_send_nack_wait() {
    if (I2CAcknowledgeHasCompleted(I2C1)) {
        poll_tc74_devices_state = tc74_devices_stop;
    }
}

static void tc74_devices_stop() {
    tc74_timeout_state = tc74_devices_loop;
#ifdef MYTC74DEBUG
    strcat(tc74_msg, " STOP");
#endif
    I2CStop(I2C1);

    poll_tc74_devices_state = tc74_devices_stop_wait;
}

static void tc74_devices_stop_wait() {
    if (I2CGetStatus(I2C1) & I2C_STOP) {
        poll_tc74_devices_state = tc74_devices_loop;
    }
}

static void tc74_devices_loop() {
    
    stop_timer(&tc74_timeout_timer);
#ifdef MYTC74DEBUG
    mylog(tc74_msg);
#endif
    tc74_i = tc74_i + 1;

    if (!(tc74_i < NMBR_OF_TC74_DEVICES)) {
#ifdef MYTC74DEBUG
        dump_tc74_device_table();
#endif
        tc74_i = 0;
        poll_tc74_devices_state = tc74_devices_idle;
    }
    else {
#ifdef MYTC74DEBUG
        strcpy(tc74_msg, "");
#endif    
        tc74_timeout_state = tc74_devices_stop;
        start_timer(&tc74_timeout_timer, tc74_timeout_callback, 100);
        poll_tc74_devices_state = tc74_devices_start;
    }
}

#ifdef MYTC74DEBUG
void dump_tc74_device_table() {
    unsigned int i;
    char msg[99];

    for (i = 0; i < NMBR_OF_TC74_DEVICES; i++) {
        sprintf(msg, "%s 0x%02x %5s %5s %5.1f", tc74_devices[i].name,
            tc74_devices[i].address,
            (tc74_devices[i].is_initialized ? "true" : "false"),
            (tc74_devices[i].is_valid_temp ? "true" : "false"),
            tc74_devices[i].temp_c);
        mylog(msg);
    };
}
#endif
