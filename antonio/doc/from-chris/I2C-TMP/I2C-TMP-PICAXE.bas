' An example program to test I2C-TMP Digital Temperature Sensor.
' The program takes temperature reading every one second.
' The program work directly with I2C-TMP module.
' The program is written for address 0x90
'
' Note: This program was tested using PICAXE 28X-1 Firmware version 2
'
' Document: TMP175 datasheet
' Updated: September 1, 2008
' E-mail: support@gravitech.us
' Gravitech
' (C) Copyright 2008 All Rights Reserved
'************************************************************

symbol I2C_ADDR = $90		' I2C address

symbol P_N     = bit0      ' Bit flag for Positive and Negative
symbol TempHi  = b1			' Variable hold data high byte
symbol TempLo  = b2    		' Variable hold data low byte
symbol Decimal = w3    		' Variable hold decimal value



MAIN:

  	PAUSE 1000

	i2cslave I2C_ADDR, i2cslow, i2cbyte
  	writei2c 1,(%01100000)		' Setup configuration register
                                           ' 12-bit
  	writei2c (0)               ' Setup Pointer Register to 0

ReadTemperature:

  	PAUSE 1000
  	i2cslave I2C_ADDR, i2cslow, i2cbyte
  	readi2c (TempHi, TempLo)   ' Read temperature high byte and low byte
  	GOSUB Cal_temp
  	
  	' Display temperature
  	sertxd ("The temperature is ")
   
   IF P_N = 0 THEN 
   	sertxd ("-")
   ENDIF
   
 	sertxd (#TempHi,".")
 	sertxd (#Decimal)
 	sertxd (" degree C",CR,LF)
 	
 	GOTO ReadTemperature


'**************************************************************
' Calculate temperature subroutine
'**************************************************************

Cal_temp:

  IF bit15 = 1 THEN     		' Check bit7 of TempHi
    P_N = 0							' If P_N = 0 means temperature is neg.
  ELSE                        ' Else temperature is pos.
    P_N = 1
  ENDIF

  TempHi = TempHi & %01111111   ' Remove sign
  TempLo = TempLo & %11110000   ' Filter out last nibble
  TempLo = TempLo >>4            ' Shift right 4 times
  Decimal = TempLo
  Decimal = Decimal * 625       ' Each bit = 0.0625 degree C
  RETURN

END

