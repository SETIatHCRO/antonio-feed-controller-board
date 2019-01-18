
#include <stdint.h>         /* For uint32_t definition                        */
#include <stdbool.h>        /* For true/false definition     */
#include <stdlib.h>
#include <stdio.h>          // sprintf(), etc)
#include <string.h>

#include "diode.h"
#include "rimbox.h"
#include "adc.h"
#include "autostart.h"

extern void (*poll_auto_start)();

extern char *EOL;

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

struct diode_entry * get_diode_table() {
    
    return diode_table;
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

#if 0
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
#endif