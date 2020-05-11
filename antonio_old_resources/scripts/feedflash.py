import getopt
import sys
import socket
import threading

host = '127.0.0.1'
port = 1518
filename = None

SOH = 0x01
DLE = 0x10
EOT = 0x04

crc_table = (0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
             0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef)

def usage ():
    print('python feedflash.py -h host -p port -f filename.hex')

try:
    opts, args = getopt.getopt(sys.argv[1:], 'h:p:f:')
    for o,a in opts:
        if o in ('-h'):
            host = a 
        if o in ('-p'):
            port = a 
        if o in ('-f'):
            filename = a
except getopt.GetoptError:
    usage()
    sys.exit()

if ((host == None) or (port == None) or (filename == None)):
    usage()
    sys.exit()

def calc_crc(chars):
    crc = 0
    for i in range(len(chars)):
        data = chars[i]
        i = (crc >> 12) ^ (data >> 4)
        crc = crc_table[i & 0x0f] ^ (crc << 4)
        i = (crc >> 12) ^ (data >> 0)
        crc = crc_table[i & 0x0f] ^ (crc << 4)
    return ([(crc & 0x00ff), ((crc >> 8) & 0x00ff)])

def escape(chars):
    escaped_chars = []
    for i in range(len(chars)):
        if ((chars[i] == SOH) or (chars[i] == DLE) or (chars[i] == EOT)):
            escaped_chars.append(DLE)
        escaped_chars.append(chars[i])
    return (escaped_chars)

def format_request(msg):
    msg_chars = []
    request_msg = ""
    for i in range(0, len(msg), 2):
        msg_chars.append(int(msg[i:i+2], 16))
    msg_chars.extend(calc_crc(msg_chars))
#    msg_chars = escape(msg_chars)
    msg_chars.insert(0, SOH)
    msg_chars.append(EOT)
    for i in range(len(msg_chars)):
        request_msg += '{0:02x}'.format(msg_chars[i])
    return (request_msg)

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
            return (rspns)
try:
    f = open(filename)
except Exception, e:
    print ('unable to open ' + filename)
    sys.exit(1)

sckt = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sckt.settimeout(5.0)
sckt.connect((host, int(port)))

request = format_request('01')
print (request)
sckt.send(request + '\r')
rspns = wait_for_response()
print (rspns)

print ('')

request = format_request('02')
print (request)
sckt.send(request + '\r')
rspns = wait_for_response()
print (rspns)

print ('')

for line in f:
    request = format_request('03' + line.strip()[1:])
    print (request)
    sckt.send(request + '\r')
    rspns = wait_for_response()
    print (rspns)


