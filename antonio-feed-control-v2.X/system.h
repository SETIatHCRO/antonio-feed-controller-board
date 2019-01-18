#ifndef SYSTEM_DEFINES_H
#define	SYATEM_DEFINES_H

/******************************************************************************/
/* System Level #define Macros                                                */
/******************************************************************************/

/* TODO Define system operating frequency */

/* Microcontroller MIPs (FCY) */
#define SYS_FREQ     80000000L
#define FCY          SYS_FREQ

#define PB_FREQ (SYS_FREQ / 4)

#define CORE_TIMER_PERIOD (SYS_FREQ / 2 / 1000)

extern volatile uint32_t tick_count;

/******************************************************************************/
/* System Function Prototypes                                                 */
/******************************************************************************/

/* Custom oscillator configuration funtions, reset source evaluation
functions, and other non-peripheral microcontroller initialization functions
go here. */

uint32_t GetTickCount();

#endif