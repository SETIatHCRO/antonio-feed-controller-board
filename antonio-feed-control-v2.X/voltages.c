#include <plib.h>           /* Include to use PIC32 peripheral libraries      */

#include <stdio.h>
#include <string.h>
#include <stdio.h>

#include "rimbox.h"
#include "adc.h"

extern char *EOL;

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
