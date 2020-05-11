import getopt
import sys
import serial
import threading

port = None
baudrate = None

def usage ():
    print('python feedshell.py -p port -b baudrate')

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

rspns = ''

def serial_to_terminal():
    global rspns
    while True:
        rspns_char = ser.read()
        if ((ord(rspns_char) == 13) or (rspns_char == '|')):
            print (rspns)
            rspns = '' 
        else:
            if (not(ord(rspns_char) == 10)):
                rspns = rspns + rspns_char

ser = serial.Serial(port, baudrate)

thrd = threading.Thread(target=serial_to_terminal, args=[])
thrd.daemon = True
thrd.start()

while True:
    try:
        line = sys.stdin.readline()
        line = line.strip()
        ser.write(line)
        ser.write('\r')
        ser.flush()
    except KeyboardInterrupt:
        break

