
//#define mLED              LATEbits.LATE0

//#define BlinkLED() (mLED = ((ReadCoreTimer() & 0x0800000) == 0))
//#define InitLED() do{DDPCON = 0; TRISE = 0xfff8; LATE= 0xffff;}while(0)
// Switch ON all the LEDs to indicate Error.
//#define Error()   (LATE = 0xffF8)

//#define ReadSwitchStatus() (PORTReadBits(IOPORT_E, BIT_7) & BIT_7)