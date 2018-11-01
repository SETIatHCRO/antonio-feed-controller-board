import getopt
import sys
import serial
import random

from time import sleep

port = None
baudrate = 4800

MAX_POWER = 240.0
MIN_POWER = 70.0

crnt_power = (MIN_POWER + MAX_POWER) / 2.0 

def usage ():
    print('python cryosim.py -p port -b baudrate')

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

def send_response(rspns):
    print (rspns.replace("\r", "\r\n"))
    ser.write(rspns)
    ser.write("\r")
    ser.flush()

def process_command():
   global cmnd
   global crnt_power

   cmnd = cmnd.strip()
   if (not(len(cmnd) > 0)):
       return
   print (cmnd)

   sleep (0.010)

   if (cmnd == "TC"):
       send_response("TC")
       send_response("{0:06.2f}".format(65.2 + (random.randint(0, 2) * 0.1)))

   if (cmnd == "E"):
       send_response("E")
       crnt_power = crnt_power + (random.randint(-10, 10) * 0.1)
       if (crnt_power < MIN_POWER):
           crnt_power = MIN_POWER
       if (crnt_power > MAX_POWER):
           crnt_power = MAX_POWER
       send_response("{0:06.2f}\r{1:06.2f}\r{2:06.2f}".format(MAX_POWER, MIN_POWER, crnt_power))

   if (cmnd == "SET SSTOP=0"):
       send_response("000.00")

   if (cmnd == "SET SSTOP=1"):
       send_response("001.00")

   cmnd = ""

while True:
    cmnd_char = ser.read()
    if ((ord(cmnd_char) == 13) or (ord(cmnd_char) == 10)):
        process_command() 
    else:
        cmnd = cmnd + cmnd_char

