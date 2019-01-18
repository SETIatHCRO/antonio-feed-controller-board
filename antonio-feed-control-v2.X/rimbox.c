
#ifdef __XC32
    #include <xc.h>          /* Defines special funciton registers, CP0 regs  */
#endif

#include <plib.h>           /* Include to use PIC32 peripheral libraries      */
#include <stdbool.h>        /* For true/false definition     */
#include <string.h>
#include <ctype.h>

#include "rimbox.h"
#include "cryo.h"
#include "vacuum.h"
#include "fan.h"
#include "temperature.h"
#include "diode.h"
#include "accel_1.h"
#include "relay.h"
#include "file_utils.h"
#include "voltages.h"
#include "commands.h"

extern bool echo_mode;

extern char *EOL;

#define MAX_ARGS 99

#define MAX_COMMAND_LEN 999
#define MAX_RESPONSE_LEN 999

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

void compute_checksum_if_requested(char *command);
bool is_command_uppercase(char *command);

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

    command = (char *)strtok(rimbox_command, " ");
    if (command == NULL) {
        poll_recv_from_rimbox = purge_chars_from_rimbox;
        return;
    }

    args[args_i] = (char *)strtok(NULL, " ");
    while (!(args[args_i] == NULL)) {
        args_i = args_i + 1;
        args[args_i] = (char *)strtok(NULL, " ");
    }

    struct command_pair *commands = get_commands();
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