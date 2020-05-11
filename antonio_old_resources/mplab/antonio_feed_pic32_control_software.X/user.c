/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

#ifdef __XC32
    #include <xc.h>          /* Defines special funciton registers, CP0 regs  */
#endif

#include <plib.h>            /* Include to use PIC32 peripheral libraries     */
#include <stdint.h>          /* For uint32_t definition                       */
#include <stdbool.h>         /* For true/false definition                     */
#include "user.h"            /* variables/params used by user.c               */
#include "system.h"

/******************************************************************************/
/* User Functions                                                             */
/******************************************************************************/

/* TODO Initialize User Ports/Peripherals/Project here */

void InitApp(void)
{
    /* Setup analog functionality and port direction */

    mPORTESetPinsDigitalOut(BIT_0);  // Yellow LED
    mPORTESetPinsDigitalOut(BIT_1);  // Red LED
    mPORTESetPinsDigitalOut(BIT_2);  // White LED
//  mPORTESetPinsDigitalOut(BIT_3);  // Green USB LED

    mPORTESetPinsDigitalOut(BIT_4);  // I2C power control
   
    /* Initialize peripherals */

    mPORTEClearBits(BIT_4);  // I2C power off

    mPORTESetBits(BIT_0 | BIT_1 | BIT_2 | BIT_3);  // LEDs off

    mPORTGClearBits(BIT_0);  // relay off
    mPORTGSetPinsDigitalOut(BIT_0);  // relay control pin
    mPORTGClearBits(BIT_0);  // relay off

    // cryo stop
    //TRISBbits.TRISB12 = 0;
    //PORTBbits.RB12 = 1;
    mPORTBClearBits(BIT_12);
    mPORTBSetPinsDigitalOut(BIT_12);
    //mPORTBSetBits(BIT_12);
    mPORTBClearBits(BIT_12);

    // cryo at temp input
    mPORTFSetPinsDigitalIn(BIT_13);

//    OpenCoreTimer(CORE_TIMER_PERIOD);
//    mConfigIntCoreTimer((CT_INT_ON | CT_INT_PRIOR_2 | CT_INT_SUB_PRIOR_0));

    // UART1 connects to the rimbox
    UARTConfigure       (UART1, UART_ENABLE_PINS_TX_RX_ONLY);
    UARTSetFifoMode     (UART1, UART_INTERRUPT_ON_RX_NOT_EMPTY);
    UARTSetLineControl  (UART1, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1 );
    UARTSetDataRate     (UART1, PB_FREQ, 19200) ;
    // UART_PERIPHERAL or UART_ENABLE ?
    UARTEnable          (UART1, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));

    // UART2 connects to the cryo controller
    UARTConfigure       (UART2, UART_ENABLE_PINS_TX_RX_ONLY);
    UARTSetFifoMode     (UART2, UART_INTERRUPT_ON_RX_NOT_EMPTY);
    UARTSetLineControl  (UART2, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1 );
    UARTSetDataRate     (UART2, PB_FREQ, 4800) ;
    UARTEnable          (UART2, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));

    // UART3 connects to the vacuum drive
    UARTConfigure       (UART3, UART_SUPPORT_IEEE_485);
    UARTSetFifoMode     (UART3, UART_INTERRUPT_ON_RX_NOT_EMPTY);
    UARTSetLineControl  (UART3, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1 );
    UARTSetDataRate     (UART3, PB_FREQ, 9600) ;
    UARTEnable          (UART3, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));

/*
 * code below was working, but decided to poll the UARTs.  Keeping
 * this as example because will likely use an interrupt handler
 * with the accelerometer.
 *
//  INTSetVectorPriority(INT_VECTOR_UART(UART3), INT_PRIORITY_LEVEL_3);
//  INTSetVectorSubPriority(INT_VECTOR_UART(UART3), INT_SUB_PRIORITY_LEVEL_0);
//  INTEnable(INT_SOURCE_UART_RX(UART3), INT_ENABLED);
 */

/*
 * decided to poll the accelerometer because an occasional overrun is
 * O.K. and I2C protocol blocking in the interrupt handler was dissatisfying.
 *
//    INTSetVectorPriority(INT_VECTOR_EX_INT(1), INT_PRIORITY_LEVEL_3);
//    INTSetVectorSubPriority(INT_VECTOR_EX_INT(1), INT_SUB_PRIORITY_LEVEL_0);
//    INTClearFlag(INT_SOURCE_EX_INT(1));
//    INTEnable(INT_SOURCE_EX_INT(1), INT_ENABLED);
 */
    
    // I2C1 interfaces with the TC74 temperature sensors
    I2CConfigure(I2C1, I2C_ENABLE_SMB_SUPPORT);
    I2CSetFrequency(I2C1, PB_FREQ, 50000);  // datasheet spec 10 - 100 kHz
    I2CEnable(I2C1, TRUE);

    // Timer 2 drives the 4-wire fan PWM
    // TODO: eliminate hard-coding of divisor value 800
    OpenTimer2(T2_ON | T2_PS_1_2 | T2_SOURCE_INT, 800);
    OpenOC2(OC_ON | OC_TIMER_MODE16 | OC_TIMER2_SRC |
            OC_CONTINUE_PULSE | OC_LOW_HIGH, 800, 400);

    CloseADC10();
    // AN2 - 24V
    // AN3 - LakeShore diode
    // AN4 - vacuum guage
    // AN5 - 48V
    OpenADC10(
        ADC_MODULE_ON | ADC_FORMAT_INTG | ADC_CLK_MANUAL |
            ADC_AUTO_SAMPLING_OFF,
        ADC_VREF_EXT_AVSS | ADC_OFFSET_CAL_DISABLE | ADC_SCAN_OFF |
            ADC_SAMPLES_PER_INT_1 | ADC_ALT_BUF_OFF | ADC_ALT_INPUT_OFF,
        ADC_CONV_CLK_PB | ADC_CONV_CLK_20Tcy,
        SKIP_SCAN_ALL,
        ENABLE_AN2_ANA | ENABLE_AN3_ANA | ENABLE_AN4_ANA | ENABLE_AN5_ANA);
    EnableADC10();

    OpenTimer3(T3_ON | T3_PS_1_256 | T3_SOURCE_INT, 0xffff);
    OpenCapture1(IC_ON | IC_CAP_16BIT | IC_TIMER3_SRC | IC_EVERY_4_RISE_EDGE);

    // I2C2 interfaces with the accelerometer
    I2CConfigure(I2C2, I2C_ENABLE_SMB_SUPPORT);
    I2CSetFrequency(I2C2, PB_FREQ, 50000);  // datasheet spec 100 - 400 kHz
    I2CEnable(I2C2, TRUE);

    SpiChnOpen(1, (SPI_OPEN_MSTEN | SPI_OPEN_CKP_HIGH), 4);

    mPORTDSetPinsDigitalOut(BIT_11);  // external flash reset
    mPORTDSetBits(BIT_11);

    mPORTDSetPinsDigitalOut(BIT_9);  // SPI SS
    mPORTDSetBits(BIT_9);
}
