# antonio-feed-controller-board
Antonio Feed Controller Board files from Rob Ackermann, project take over by Jon Richards in November 2018.

The original firmware project in antonio/mplab/antonio_feed_pic32_control_software.X has been ported to the newest version of the MPLab IDE and compiler. This new project is in ./antonio-feed-control-v2.X. The original project remains untouched for prosperity. The new port has been modified by broken into smaller .c files to make the code more readable.

Requirements for the feed controller project, as of Jan 18, 2019

  - MPLab IDE 5.10
  - Compiler v 2.15
  - Windows 10
  - Pickit4 programmer

## Useful Links

The processor is the Microchip 32MX795F512L
  - Product page: https://www.microchip.com/wwwproducts/en/PIC32MX795F512L
  - Datasheet http://ww1.microchip.com/downloads/en/DeviceDoc/60001156J.pdf
  - PIC32MX family reference manual: http://hades.mech.northwestern.edu/images/2/21/61132B_PIC32ReferenceManual.pdf
  - Falsh programming spec: http://ww1.microchip.com/downloads/en/DeviceDoc/PIC32-Flash-%20Programming-%20Specification-DS60001145W.pdf
  - Compiler User's Guide: http://hades.mech.northwestern.edu/images/c/cc/XC32_Compiler_Users_Guide.pdf

## Notes

The on-board temperature sensor sampled periodically since summer 2018 shows a variation of -6C to 46C degrees.

There are 3 UARTS, parameters defines in user.c

  - UART1, rimbox baud 19200
  - UART2, cryo contriller baud 4800
  - UART3, vacuum drive baud 9600

## I2C 

  - I2C1, TC74 temperature sensors, set to freq 50,000
  - I2C2, accelerometer, set to freq 50,000
  - Cabling info: https://www.analog.com/media/en/technical-documentation/technical-articles/I2C-Cabling.pdf
  - More cabling info: https://hackaday.com/2017/02/08/taking-the-leap-off-board-an-introduction-to-i2c-over-long-wires/


## Oscillator

  - Internal vs exteranl oscillator info: https://www.sciencedirect.com/topics/computer-science/internal-oscillator
  - Microchip Crystal Oscillator Basics and Crystal Selection http://ww1.microchip.com/downloads/en/appnotes/00826a.pdf
  - AN2291 - Internal Oscillator Calibration Using the Temperature Indicator Module (16bit PIC, but info is relevant) https://www.microchip.com/stellent/groups/picmicro_sg/documents/appnotes/jp588536.pdf
  - The OSCTUNE register can be used to adjust the internal oscillator frequency during run time. 
  - Oscillator explanation and link to oscillator spreadsheet: http://microchipdeveloper.com/32bit:osc-overview
  - AN244 - Internal RC Oscillator Calibration - 16bit - http://ww1.microchip.com/downloads/en/AppNotes/00244a.pdf

## Accerometer

  - The accelerometer used: https://www.sparkfun.com/products/12756
  - Tech manual: https://www.nxp.com/docs/en/data-sheet/MMA8452Q.pdf
  - Board wiring: https://github.com/SETIatHCRO/antonio-feed-controller-board/blob/master/Items%20collected%20by%20MCF/accel.pdf




