import getopt
import sys
import time
import datetime
import socket
import threading

host = '127.0.0.1'
port = 1518

def usage ():
    print('python feedmon.py -h host -p port')

try:
    opts, args = getopt.getopt(sys.argv[1:], 'h:p:')
    for o,a in opts:
        if o in ('-h'):
            host = a 
        if o in ('-p'):
            port = a 
except getopt.GetoptError:
    usage()
    sys.exit()

if ((host == None) or (port == None)):
    usage()
    sys.exit()

def wait_for_response():
    rspns = ""
    while (True):
        try:
            rspns_chars = sckt.recv(99)
            for i in range(len(rspns_chars)):
                if (ord(rspns_chars[i]) == 0x0d):
                    return (rspns)
                rspns += rspns_chars[i]
        except Exception, e:
            return ('timeout (local)')

sckt = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sckt.settimeout(1.0)
sckt.connect((host, int(port)))

while (True):
    print ('****************************************************************')
    ts = int(time.time())
    print(datetime.datetime.fromtimestamp(ts).strftime('%Y-%m-%d %H:%M:%S'))
    print (ts)
#    request = '<TP OP="GT" LC="MS"/>'
#    print (request)
#    sckt.send(request + '\r')
#    rspns = wait_for_response()
#    print (rspns)
    request = '0010031602=?###'
    print (request)
    sckt.send(request + '\r')
    rspns = wait_for_response()
    print (rspns)
    time.sleep(0.5)
    request = '0010039802=?###'
    print (request)
    sckt.send(request + '\r')
    rspns = wait_for_response()
    print (rspns)
    time.sleep(0.5)
    request = '0010033002=?###'
    print (request)
    sckt.send(request + '\r')
    rspns = wait_for_response()
    print (rspns)
    time.sleep(0.5)
    request = '0010034602=?###'
    print (request)
    sckt.send(request + '\r')
    rspns = wait_for_response()
    print (rspns)
    time.sleep(0.5)
    request = 'TC'
    print (request)
    sckt.send(request + '\r')
    rspns = wait_for_response()
    print (rspns)
    time.sleep(0.5)
    request = 'P'
    print (request)
    sckt.send(request + '\r')
    rspns = wait_for_response()
    print (rspns)
    sys.stdout.flush()
    request = 'getadc 4'
    print (request)
    sckt.send(request + '\r')
    rspns = wait_for_response()
    print (rspns)
    sys.stdout.flush()
    time.sleep(60)

