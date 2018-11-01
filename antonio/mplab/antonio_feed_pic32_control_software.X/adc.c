#include <plib.h>           /* Include to use PIC32 peripheral libraries      */
#include <stdio.h>
#include <stdint.h>         /* For uint32_t definition                        */
#include <stdbool.h>        /* For true/false definition     */
#include <stdlib.h>
#include <math.h>

#include "oneshot.h"
#include "accel.h"
#include "main.h"
#include "adc.h"

static unsigned int adc_i = 0;

struct adc adc_inputs[] = {
    {"AN2", 02, false, 0.0},
    {"AN3", 03, false, 0.0},
    {"AN4", 04, false, 0.0},
    {"AN5", 05, false, 0.0}
};

#define NMBR_OF_ADC_INPUTS (sizeof (adc_inputs) / sizeof(struct adc))

static struct oneshot_timer adc_start_timer = {"adc_start_timer", 0, NULL, NULL};
static struct oneshot_timer adc_acquire_timer = {"adc_acquire_timer", 0, NULL, NULL};

static void (*poll_adc_state)();

void init_adc() {
    poll_adc_state = adc_idle;

    start_timer(&adc_start_timer, adc_start_callback, 100);
}

unsigned int getadctable(struct adc **adctblptr) {
    *adctblptr = adc_inputs;

    return (NMBR_OF_ADC_INPUTS);
}

static void adc_start_callback() {
    if (poll_adc_state == adc_idle) {
        poll_adc_state = adc_acquire;
    }

    start_timer(&adc_start_timer, adc_start_callback, 100);
}

static void adc_convert_callback() {
    poll_adc_state = adc_convert;
}

void poll_adc() {
    poll_adc_state();
}

static void adc_idle() {

}

static void adc_acquire() {
    if (adc_inputs[adc_i].address == 2) {
        SetChanADC10(ADC_CH0_NEG_SAMPLEA_NVREF | ADC_CH0_POS_SAMPLEA_AN2);
    }
    if (adc_inputs[adc_i].address == 3) {
        SetChanADC10(ADC_CH0_NEG_SAMPLEA_NVREF | ADC_CH0_POS_SAMPLEA_AN3);
    }
    if (adc_inputs[adc_i].address == 4) {
        SetChanADC10(ADC_CH0_NEG_SAMPLEA_NVREF | ADC_CH0_POS_SAMPLEA_AN4);
    }
    if (adc_inputs[adc_i].address == 5) {
        SetChanADC10(ADC_CH0_NEG_SAMPLEA_NVREF | ADC_CH0_POS_SAMPLEA_AN5);
    }

    AcquireADC10();

    start_timer(&adc_acquire_timer, adc_convert_callback, 10);
    poll_adc_state = adc_acquire_wait;
}

static void adc_acquire_wait() {

}

static void adc_convert() {
    ConvertADC10();

    poll_adc_state = adc_done_read;
}

static void adc_done_read() {
    unsigned int adc_value;

    if (BusyADC10()) { // i.e., AD1CON1 DONE bit is 1 (conversion done)
        adc_value = ReadADC10(0);
        if (!(adc_inputs[adc_i].is_valid_value)) {
            adc_inputs[adc_i].value = (float) adc_value;
            adc_inputs[adc_i].is_valid_value = true;
        }
        adc_inputs[adc_i].value =
            (0.9 * adc_inputs[adc_i].value) + (0.1 * adc_value);
        poll_adc_state = adc_loop;
    }
}

static void adc_loop() {
    adc_i = adc_i + 1;

    if (!(adc_i < NMBR_OF_ADC_INPUTS)) {
#ifdef MYADCDEBUG
        dump_adc_table();
#endif
        adc_i = 0;
        poll_adc_state = adc_idle;
    }
    else {
        poll_adc_state = adc_acquire;
    }
}

#ifdef MYADCDEBUG
void dump_adc_table() {
    unsigned int i;
    char msg[99];

    for (i = 0; i < NMBR_OF_ADC_INPUTS; i++) {
        sprintf(msg, "%s %02u %5s %6.1f", adc_inputs[i].name,
            adc_inputs[i].address,
            (adc_inputs[i].is_valid_value ? "true" : "false"),
            adc_inputs[i].value);
        mylog(msg);
    }
}
#endif
