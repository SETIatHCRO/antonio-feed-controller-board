#include <stdbool.h>        /* For true/false definition     */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tc74.h"
#include "temperature.h"
#include "rimbox.h"

extern char *EOL;

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