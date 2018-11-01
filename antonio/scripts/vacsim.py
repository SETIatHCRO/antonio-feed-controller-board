import getopt
import sys
import serial
import random

from time import sleep

port = None
baudrate = 9600

def usage ():
    print('python vacsim.py -p port -b baudrate')

try:
    opts, args = getopt.getopt(sys.argv[1:], 'p:b:')
    for o,a in opts:
        if o in ('-p'):
            port = a 
        if o in ('-b'):
            baudrate = a 
except getopt.GetoptError:
    usage()
    sys.exit()

if ((port == None) or (baudrate == None)):
    usage()
    sys.exit()

ser = serial.Serial(port, baudrate)

cmnd = ""

def checksum(response):
    sum = 0

    for i in range(len(response)):
        sum = sum + ord(response[i])
    sum = sum % 256

    return ('{0:03d}'.format(sum))

def send_response(rspns):
    print (rspns)
    ser.write(rspns)
    ser.write("\r")
    ser.flush()

def process_command():
   global cmnd
   print (cmnd)

   sleep (0.010)

   address = cmnd[0:3]
   action = cmnd[3:5]
   vacparm = cmnd[5:8]
   datalen = cmnd[8:10]
   dataval = cmnd[10:(10+int(datalen))]

   if ((action == "10") and (vacparm == "009") and (datalen == "06")):
       response = address + "10" + vacparm + "06" + dataval 
       response = response + checksum(response)
       send_response(response)

   if ((action == "10") and (vacparm == "010") and (datalen == "06")):
       response = address + "10" + vacparm + "06" + dataval 
       response = response + checksum(response)
       send_response(response)

   if ((action == "10") and (vacparm == "700") and (datalen == "06")):
       response = address + "10" + vacparm + "06" + dataval 
       response = response + checksum(response)
       send_response(response)

   if ((action == "10") and (vacparm == "023") and (datalen == "06")):
       response = address + "10" + vacparm + "06" + dataval 
       response = response + checksum(response)
       send_response(response)

   if ((action == "10") and (vacparm == "024") and (datalen == "03")):
       response = address + "10" + vacparm + "03" + dataval 
       response = response + checksum(response)
       send_response(response)

   if ((action == "10") and (vacparm == "025") and (datalen == "03")):
       response = address + "10" + vacparm + "03" + dataval 
       response = response + checksum(response)
       send_response(response)

   if ((action == "10") and (vacparm == "035") and (datalen == "03")):
       response = address + "10" + vacparm + "03" + dataval 
       response = response + checksum(response)
       send_response(response)

   if ((action == "00") and (vacparm == "398") and (datalen == "02")):
       response = address + "10" + vacparm + "06" + "{0:06d}".format(90015 + random.randint(-3, 3))
       response = response + checksum(response)
       send_response(response)

   if ((action == "00") and (vacparm == "310") and (datalen == "02")):
       response = address + "10" + vacparm + "06" + "000050"
       response = response + checksum(response)
       send_response(response)

   if ((action == "00") and (vacparm == "316") and (datalen == "02")):
       response = address + "10" + vacparm + "06" + "000012"
#      response = address + "10" + vacparm + "06" + "000024"
       response = response + checksum(response)
       send_response(response)

   if ((action == "00") and (vacparm == "326") and (datalen == "02")):
       response = address + "10" + vacparm + "06" + "000030"
       response = response + checksum(response)
       send_response(response)

   if ((action == "00") and (vacparm == "330") and (datalen == "02")):
       response = address + "10" + vacparm + "06" + "000041"
       response = response + checksum(response)
       send_response(response)

   if ((action == "00") and (vacparm == "342") and (datalen == "02")):
       response = address + "10" + vacparm + "06" + "000052"
       response = response + checksum(response)
       send_response(response)

   if ((action == "00") and (vacparm == "346") and (datalen == "02")):
       response = address + "10" + vacparm + "06" + "000063"
       response = response + checksum(response)
       send_response(response)

   cmnd = ""

while True:
    cmnd_char = ser.read()
    if (ord(cmnd_char) == 13):
        process_command() 
    else:
        if (not(ord(cmnd_char) == 10)):
            cmnd = cmnd + cmnd_char

