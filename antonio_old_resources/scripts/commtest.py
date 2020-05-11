import getopt
import sys
import serial

port = None
speed = None

def usage ():
    print('python commtest.py -p port -s speed')

try:
    opts, args = getopt.getopt(sys.argv[1:], 'p:s:')
    for o,a in opts:
        if o in ('-p'):
            port = a 
        elif o in ('-s'):
            speed = a 
        else:
            usage()
            sys.exit()
except getopt.GetoptError:
    usage()
    sys.exit()

if ((port == None) or (speed == None)):
    usage()
    sys.exit()

serialport = serial.Serial(port, speed, timeout=0.5)
serialport.write("hello")
response = serialport.readlines(None)
print (response)
