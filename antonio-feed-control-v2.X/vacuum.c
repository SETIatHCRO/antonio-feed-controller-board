

#include <plib.h>           /* Include to use PIC32 peripheral libraries      */

#include <stdbool.h>        /* For true/false definition     */
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "vacuum.h"
#include "rimbox.h"
#include "adc.h"
#include "oneshot.h"

extern char *EOL;
extern char TIMEOUT[];
extern char *command;

#define MAX_COMMAND_LEN 99
#define MAX_RESPONSE_LEN 99

unsigned int vac_req_i = 0;
char vac_request[MAX_COMMAND_LEN];

unsigned int vac_rspns_i = 0;
char vac_response[MAX_RESPONSE_LEN];

bool is_vac_busy = false;

bool is_vac_response_ready = false;

bool is_parse_vacuum_response = false;

unsigned int vacuum_address = 1;

struct oneshot_timer vac_rspns_timeout_timer = {"vac_response_timeout", 0, NULL, NULL};

void compute_checksum_if_requested(char *command);

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


void wait_vacuum_not_busy() {
    if (is_vac_busy) {
        return;
    }
    
    send_command_to_vacuum(command);
 
    
    //JR - for testing
    /*
    free_vacuum_session();
    poll_vac_session = vac_poll_idle;
    return;
     */
    
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
    
    UARTSendBreak(UART3);
    UARTSendDataByte(UART3,'A'); 
    is_vac_response_ready = true;

    poll_vac_session = vac_poll_idle;
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