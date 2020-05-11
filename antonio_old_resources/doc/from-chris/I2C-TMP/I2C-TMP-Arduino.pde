/******************************************************************************
Example program I2C-TMP interface with Arduino.

SETUP:    I2C-TMP => Arduino
          PIN3 => ground, PIN4 => A5(SCL), PIN5 => A4(SDA), PIN6 => +5V
Note:     The program is written for address 0x90 (Arduino address 0x48).
          This program was tested using Arduino Nano
Document: TMP175 datasheet
Updated:  September 4, 2008
E-mail:   support@gravitech.us
          Gravitech
(C) Copyright 2008 All Rights Reserved
*******************************************************************************/

#include <Wire.h> 

byte TempHi;              // Variable hold data high byte
byte TempLo;              // Variable hold data low byte
boolean P_N;              // Bit flag for Positive and Negative
unsigned int Decimal;     // Variable hold decimal value

void Cal_Temp();
/*******************************************************************************
                      Setup
*******************************************************************************/ 
void setup() 
{ 
  Serial.begin(9600);
  Wire.begin();             // join i2c bus (address optional for master) 
  delay(1000);
} 
 
/*******************************************************************************
                      Main Loop
*******************************************************************************/  
void loop() 
{
  const int I2C_address = 0x48;  // I2C write address 
    
  delay(100);
  Wire.beginTransmission(I2C_address);
  Wire.send(1);             // Setup configuration register
  Wire.send(0x60);          // 12-bit
  Wire.endTransmission(); 
  
  Wire.beginTransmission(I2C_address);
  Wire.send(0);             // Setup Pointer Register to 0
  Wire.endTransmission(); 
  
  while (1)
  {
    delay(1000);
    
    // Read temperature value
    Wire.requestFrom(I2C_address, 2);
    while(Wire.available())          // Checkf for data from slave
    {                                
      TempHi = Wire.receive();       // Read temperature high byte
      TempLo = Wire.receive();       // Read temperature low byte
    } 
    Cal_Temp ();
    
    // Display temperature
    Serial.print("The temperature is ");
    if (P_N == 0)
      Serial.print("-");
    Serial.print(TempHi,DEC);
    Serial.print(".");
    Serial.print(Decimal,DEC);
    Serial.println(" degree C");
  }  
}

void Cal_Temp()
{
  if (TempHi&0x80)          // If bit7 of the TempHi is HIGH then the temperature is negative
    P_N = 0;
  else                      // Else the temperature is positive
    P_N = 1;
  
  TempHi = TempHi & 0x7F;   // Remove sign
  TempLo = TempLo & 0xF0;   // Filter out last nibble
  TempLo = TempLo >>4;      // Shift right 4 times
  Decimal = TempLo;
  Decimal = Decimal * 625;  // Each bit = 0.0625 degree C
    
  
  
}
