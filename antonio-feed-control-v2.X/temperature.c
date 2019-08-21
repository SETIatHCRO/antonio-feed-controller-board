#include <stdbool.h>        /* For true/false definition     */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tc74.h"
#include "temperature.h"
#include "rimbox.h"

extern char *EOL;

/**
 * Get the temperature of a thermistor and print out
 * the result to the rimbox. a0 through a7
 * @param args the args passed in from the command processor. 
 *             Only the first index should contain a thermistor name.
 */
void gettemp_command(char *args[]) {
    char msg[16];
    float temp;
    bool is_fahrenheit = false;

    if (args[0] == NULL) {
        send_to_rimbox(EOL);
        return;
    }

    if (args[1] != NULL) {
        if (strcmp(args[1], "-f") == 0) {
            is_fahrenheit = true;
        }
        else {
            send_to_rimbox(EOL);
            return;
        }
    }
    
    temp = get_temp(args[0]);
    
    if(temp == TEMP_NOT_INITIALIZED) {
        send_to_rimbox("not initialized");
        send_to_rimbox(EOL);
        return;
    }
    
    if(temp == TEMP_INVALID_TEMP) {
        send_to_rimbox("not valid");
        send_to_rimbox(EOL);
        return;
    }
    
    if(temp == TEMP_INVALID_TEMP) {
        send_to_rimbox(EOL);
        return;
    }

    if (is_fahrenheit) {
        temp = ((9.0 / 5.0) * temp) + 32.0;
    }
    
    sprintf(msg, "%3.1f", temp);
    send_to_rimbox(msg);
    send_to_rimbox(EOL);
    return;
    
}

/**
 * Get the temperature of one of the sensors.
 * @param name the name of the thermistor. a0 through a7
 * @return temp in degrees C. ON_BOARD_TEMP_NOT_INITIALIZED or 
 *   ON_BOARD_TEMP_INVALID_TEMP
 */
float get_temp(const char *name) {
    
    int i;
    int nmbr_of_devices;
    struct tc74 *tc74_devices;
        
    nmbr_of_devices = gettc74table(&tc74_devices);

    for (i = 0; i < nmbr_of_devices; i++) {
        if (strcasecmp(name, tc74_devices[i].name) == 0) {
            if (!(tc74_devices[i].is_initialized)) {
                return TEMP_NOT_INITIALIZED;
            }
            if (!(tc74_devices[i].is_valid_temp)) {
                return TEMP_INVALID_TEMP;
            }
            return tc74_devices[i].temp_c;
        }
    }
    return TEMP_INVALID_NAME;
    
}