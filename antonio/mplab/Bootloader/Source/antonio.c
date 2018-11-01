#include <p32xxxx.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <plib.h>
#include "Include/Uart/Uart.h"
#include "Include/BootLoader.h"
#include "Include/Framework/Framework.h"
#include "Source/pff.h"
#include "Source/antonio.h"
#include "Source/base64.h"
#include "Source/adler.h"

static char CR[]   = "\r";
static char CRLF[] = "\r\n";
static char VBAR[] = "|";

static char *LINESEP = VBAR;
static char *EOL = CR;

static char prev_chr = 0;

static bool echo_mode = false;

static char init_string[] = "PIC32 Bootloader (Antonio) 1.3";

static FATFS fso;

static unsigned int flash_write_addr;

static unsigned char flash_write_sctr_bfr[4096];

#define MAX_CMND_LEN 1999
static char command[MAX_CMND_LEN];
static unsigned int cmndi = 0;

#define MAX_RSPNS_LEN 999
static char response_queue[MAX_RSPNS_LEN];
static unsigned int response_queue_back = 0;
static unsigned int response_queue_front = 0;
static unsigned int response_queue_count = 0;

typedef void (*command_function) (char *args[]);

struct command_pair {
    char *name;
    command_function function;
};

static void help_command(char *args[]);
static void ls_command(char *args[]);
static void writedisk_command(char *args[]);
static void eraseflash_command(char *args[]);
static void programflash_command(char *args[]);
static void jumptoapp_command(char *args[]);
static void hyperterminal_command(char *args[]);
static void stty_command(char *args[]);
static void reset_command(char *args[]);

struct command_pair commands[] = {
    {"help", help_command},
    {"ls", ls_command},
    {"writedisk", writedisk_command},
    {"eraseflash", eraseflash_command},
    {"programflash", programflash_command},
    {"jumptoapp", jumptoapp_command},
//  {"hyperterminal", hyperterminal_command},
    {"ht", hyperterminal_command},
    {"minex", hyperterminal_command},
    {"stty", stty_command},
    {"reset", reset_command},
};

#define NUMBER_OF_COMMANDS (sizeof (commands) / sizeof(struct command_pair))

char OK[] = "OK";
char ERROR[] = "error";
char TIMEOUT[] = "timeout";
char ACK[] = "ACK";
char NAK[] = "NAK";

//          1         2         3         4         5         6         7
//0123456789012345678901234567890123456789012345678901234567890123456789012
char *help_text[] = {
 "ls - list flash disk files",
 "writedisk <offset> <length> <chksum> <data> - write filesystem to disk",
 "eraseflash - erase CPU flash",
 "programflash <filename.HEX> - program CPU flash using disk HEX file",
 "jumptoapp - jump to main application",
 "ht or minex - echo on, line sep. CRLF",
 "stty - (default for rim box) echo off, line sep. vertical bar",
 "reset - restart bootloader"
 };

#define NMBR_OF_HELP_TEXT_LINES (sizeof(help_text) / sizeof(char *))

static void enqueue_response_char(char chr);
static void enqueue_response_msg(char *msg);
static char dequeue_response_char();
static void process_command(char *cmnd);

static void enqueue_response_char(char chr) {
    response_queue[response_queue_back] = chr;
    response_queue_back++;
    if (!(response_queue_back < MAX_RSPNS_LEN)) {
        response_queue_back = 0;
    }
    response_queue_count++;
}


static void enqueue_response_msg(char *msg) {
    unsigned int i;
    unsigned int N = strlen(msg);

    for (i = 0; i < N; i++) {
        enqueue_response_char(msg[i]);
    }
}

static char dequeue_response_char() {
    unsigned int i = response_queue_front;

    if (response_queue_count > 0) {
        response_queue_front++;
        if (!(response_queue_front < MAX_RSPNS_LEN)) {
            response_queue_front = 0;
        }
        response_queue_count--;
    }

    return (response_queue[i]);
}

void antonio_bootloader_init() {
    EnableWDT();
    ClearWDT();

    SpiChnOpen(1, (SPI_OPEN_MSTEN | SPI_OPEN_CKP_HIGH), 4);

    TRISDbits.TRISD11 = 0;
    PORTDbits.RD11 = 1;
//    mPORTDSetPinsDigitalOut(BIT_11);  // external flash reset
//    mPORTDSetBits(BIT_11);

    TRISDbits.TRISD9 = 0;
    PORTDbits.RD9 = 1;
//    mPORTDSetPinsDigitalOut(BIT_9);  // SPI SS
//    mPORTDSetBits(BIT_9);

    // cryo stop off
    TRISBbits.TRISB12 = 0;
    PORTBbits.RB12 = 1;

    // set fan to 60%
    OpenTimer2(T2_ON | T2_PS_1_1 | T2_SOURCE_INT, 800);
    OpenOC2(OC_ON | OC_TIMER_MODE16 | OC_TIMER2_SRC |
            OC_CONTINUE_PULSE | OC_LOW_HIGH, 800, 320);

    pf_mount(&fso);

    enqueue_response_msg(init_string);
    enqueue_response_msg(EOL);
}

void antonio_put_char(char chr) {
    if ((chr == 0x0d) || (chr == 0x0a)) {
        if (echo_mode) {
            if (prev_chr != 0x0d) {
                enqueue_response_msg(EOL);
            }
        }
        command[cmndi] = 0;
        if (cmndi > 0) {
            process_command(command);
        }
        cmndi = 0;
        prev_chr = chr;
        return;
    }

    if ((chr >= 0x20) && (chr <= 0x7e)) {
        if (cmndi < (MAX_CMND_LEN - 1)) {  // leave room for zero termination
            command[cmndi] = chr;
            cmndi++;
            if (echo_mode) {
                enqueue_response_char(chr);
            }
        }
    }
    
    if ((chr == 0x08) || (chr == 0x7f)) {
        if (cmndi > 0) {
            cmndi--;
            if (echo_mode) {
                enqueue_response_char(0x08);
                enqueue_response_char(' ');
                enqueue_response_char(0x08);
            }
        }
    }

    prev_chr = chr;
}

BOOL antonio_poll() {
    ClearWDT();


    if (response_queue_count > 0) {
        return (true);
    }

    return (false);
}

char antonio_get_char() {
    return (dequeue_response_char());
}

static void process_command(char *cmnd) {
#define MAX_ARGS 99

    unsigned int i;
    unsigned int cmnds_i;
    char *command;
    char *args[MAX_ARGS];

    unsigned int args_i = 0;

    command = strtok(cmnd, " ");
    if (command == NULL) {
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
            return;
        }
    }
}

static void help_command(char *args[]) {
    unsigned int i;

    for (i = 0; i < NMBR_OF_HELP_TEXT_LINES; i++) {
        if (i > 0) {
            enqueue_response_msg(LINESEP);
        }
        enqueue_response_msg(help_text[i]);
    }
    enqueue_response_msg(EOL);
}

static void ls_command(char *args[]) {
    char msg[99];

    FRESULT res;
    FILINFO fno;
    DIR dir;
    int i;
    unsigned int slen;

    res = pf_opendir(&dir, "");
    if (res == FR_OK) {
        for (i = 0;; i++) {
            res = pf_readdir(&dir, &fno);
            if ((res != FR_OK) || (fno.fname[0] == 0)) {
                break;
            }
            if (i > 0) {
                enqueue_response_msg(LINESEP);
            }
            strcpy(msg, fno.fname);
            strcat(msg, " - ");
            slen = strlen(msg);
            ultoa(&msg[slen], fno.fsize, 10);
            enqueue_response_msg(msg);
        }
    }

    enqueue_response_msg(EOL);
}

/*
 * writedisk offset length chksum base64data
 */
static void writedisk_command(char *args[]) {
    char msg[99];
    unsigned int i;

    unsigned int offset;
    unsigned int length;
    unsigned int chksum;
    unsigned int base64_len;
    unsigned int decode_len;

    char decode_bfr[MAX_CMND_LEN];

    if (args[0] == NULL) {
        enqueue_response_msg(EOL);
        return;
    }
    offset = atoi(args[0]);

    if (args[1] == NULL) {
        enqueue_response_msg(EOL);
        return;
    }
    length = atoi(args[1]);

    if (args[2] == NULL) {
        enqueue_response_msg(EOL);
        return;
    }
    chksum = xtoi(args[2]);

    if (args[3] == NULL) {
        enqueue_response_msg(EOL);
        return;
    }
    base64_len = strlen(args[3]);
//  decode_len = ((base64_len * 3) / 4) + 9;  // 3/4 plus some extra

    decode_len = base64_decode(decode_bfr, args[3]);

    if (decode_len != length) {
        enqueue_response_msg(NAK);
        enqueue_response_msg(EOL);
        return;
    }

    if (adler(decode_len, decode_bfr) != chksum) {
        enqueue_response_msg(NAK);
        enqueue_response_msg(EOL);
        return;
    }

    if ((offset > 0) && (offset != flash_write_addr)) {
        enqueue_response_msg(NAK);
        enqueue_response_msg(EOL);
        return;
    }

    if (offset == 0) {
        flash_write_addr = 0;
    }

    for (i = 0; i < decode_len; i++) {
        flash_write_sctr_bfr[(flash_write_addr & 0x0fff)] = decode_bfr[i];
        flash_write_addr++;
        if (!(flash_write_addr & 0x0fff)) {
           disk_writes(((flash_write_addr >> 12) - 1), flash_write_sctr_bfr);
        }
    }

    enqueue_response_msg(ACK);
    enqueue_response_msg(EOL);
}

static void eraseflash_command(char *args[]) {
    UINT8 cmnd[] = {0x02};

    BuildRxFrame(cmnd, 1);
    
    enqueue_response_msg(OK);
    enqueue_response_msg(EOL);
}

static void programflash_command(char *args[]) {
    char msg[99];
    unsigned int i;
    unsigned int hexi;
    unsigned int nibblei;
    char chr;
    UINT8 nibble;
    UINT txbuf_len;

#define READ_BFR_LEN 80
    char read_bfr[READ_BFR_LEN];
    UINT8 hex_bfr[READ_BFR_LEN];

    FRESULT res;
    WORD read_count;

    unsigned int record_count = 0;

    if (args[0] == NULL) {
        enqueue_response_msg(EOL);
        return;
    }

    res = pf_open(args[0]);
    if (!(res == FR_OK)) {
        enqueue_response_msg(EOL);
        return;
    }

    do {
        res = pf_read(read_bfr, (WORD) READ_BFR_LEN, &read_count);
        if (!(res == FR_OK)) {
            goto programflash_command_error;
        }
        if (read_count > 0) {
            for (i = 0; i < read_count; i++) {
                chr = tolower(read_bfr[i]);
                if (chr == ' ') {
                    continue;
                }
                if (chr == ':') {
                    hex_bfr[0] = 0x03;
                    hexi = 1;
                    nibblei = 0;
                    continue;
                }
                if ((chr == 0x0d) || (chr == 0x0a)) {
                    if (hexi > 1) {
                        if (nibblei != 0) {
                            goto programflash_command_error;
                        }
                        BuildRxFrame(hex_bfr, hexi);
                        FRAMEWORK_FrameWorkTask();
                        txbuf_len = GetTransmitFrame(hex_bfr);
                        if (!((txbuf_len > 0) && (hex_bfr[0] == 0x03))) {
                            goto programflash_command_error;
                        }
                        record_count++;
                    }
                    hexi = 0;
                    continue;
                }
                if (isxdigit(chr)) {
                    if (!((hexi >= 1) && (hexi < READ_BFR_LEN))) {
                        goto programflash_command_error;
                    }
                    if (isdigit(chr)) nibble = (UINT8) (chr - '0');
                    if (isalpha(chr)) nibble = (UINT8) ((chr - 'a') + 10);
                    if (nibblei == 0) {
                        nibblei = 1;
                        hex_bfr[hexi] = (nibble << 4);
                    }
                    else {
                        hex_bfr[hexi] |= (nibble & 0x0f);
                        hexi++;
                        nibblei = 0;
                    }
                    continue;
                }
                goto programflash_command_error;
            }
        }
    }
    while (read_count == READ_BFR_LEN);

    itoa(msg, record_count, 10);
    strcat(msg, " records");
    enqueue_response_msg(msg);
    enqueue_response_msg(EOL);
    enqueue_response_msg(OK);
    enqueue_response_msg(EOL);
    return;
programflash_command_error:
    enqueue_response_msg(ERROR);
    enqueue_response_msg(EOL);
}

static void jumptoapp_command(char *args[]) {
    UINT8 cmnd[] = {0x05};

    BuildRxFrame(cmnd, 1);
}

static void hyperterminal_command(char *args[]) {
    echo_mode = true;

    EOL = CRLF;
    LINESEP = EOL;

    enqueue_response_msg(OK);
    enqueue_response_msg(EOL);
}

static void stty_command(char *args[]) {
    echo_mode = false;

    EOL = CR;
    LINESEP = VBAR;

    enqueue_response_msg(OK);
    enqueue_response_msg(EOL);
}

static void reset_command(char *args[]) {
    SoftReset();    
}
