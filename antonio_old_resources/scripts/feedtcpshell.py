import getopt
import sys
import socket
import threading

host = "127.0.0.1"
port = 1518

def usage ():
    print('python feedtcpshell.py -h host -p port')

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

def socket_to_terminal():
    global sckt
    while True:
        rspns = sckt.recv(999)
        rspns = rspns.replace("|", "\r")
        rspns = rspns.replace("\r", "\r\n")
        sys.stdout.write(rspns)
        sys.stdout.flush()

sckt = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sckt.connect((host, int(port)))

thrd = threading.Thread(target=socket_to_terminal, args=[])
thrd.daemon = True
thrd.start()

while True:
    try:
        line = sys.stdin.readline()
        line = line.strip()
        sckt.send(line + '\r')
    except KeyboardInterrupt:
        break

