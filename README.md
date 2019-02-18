# antonio-feed-controller-board
Antonio Feed Controller Board files from Rob Ackermann, project take over by Jon Richards in November 2018.

The original firmware project in antonio/mplab/antonio_feed_pic32_control_software.X has been ported to the newest version of the MPLab IDE and compiler. This new project is in ./antonio-feed-control-v2.X. The original project remains untouched for prosperity. The new port has been modified by broken into smaller .c files to make the code more readable.

Requirements for the feed controller project, as of Jan 18, 2019

  - MPLab IDE 5.10
  - Compiler v 2.15
  - Windows 10
  - Pickit4 programmer

##Useful Links

The processor is the Microchip 32MX795F512L
  - Product page: https://www.microchip.com/wwwproducts/en/PIC32MX795F512L
  - Datasheet http://ww1.microchip.com/downloads/en/DeviceDoc/60001156J.pdf
  - PIC32MX family reference manual: http://hades.mech.northwestern.edu/images/2/21/61132B_PIC32ReferenceManual.pdf
  - Falsh programming spec: http://ww1.microchip.com/downloads/en/DeviceDoc/PIC32-Flash-%20Programming-%20Specification-DS60001145W.pdf
  - Compiler User's Guide: http://hades.mech.northwestern.edu/images/c/cc/XC32_Compiler_Users_Guide.pdf

##Notes

There are 3 UARTS, parameters defines in user.c

  - UART1, rimbox baud 19200
  - UART2, cryo contriller baud 4800
  - UART3, vacuum drive baud 9600

##I2C 

  - I2C1, TC74 temperature sensors, set to freq 50,000
  - I2C2, accelerometer, set to freq 50,000


##Oscillator

  - Internal vs exteranl oscillator info: https://www.sciencedirect.com/topics/computer-science/internal-oscillator
  - Microchip Crystal Oscillator Basics and Crystal Selection http://ww1.microchip.com/downloads/en/appnotes/00826a.pdf
  - Internal Oscillator Calibration Using the Temperature Indicator Module (16bit PIC, but info is relevant) https://www.microchip.com/stellent/groups/picmicro_sg/documents/appnotes/jp588536.pdf
  - The OSCTUN register can be used to adjust the internal oscillator frequency during run time. 



