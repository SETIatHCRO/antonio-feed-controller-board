import getopt
import sys
import time
import socket
import zlib
import base64

host = '127.0.0.1'
port = 1518
filename = None
block_size = 256 

offset = 0

def usage ():
    print('python writedisktcp.py [-h host] [-p port] [-b blocksize] -f filename.hex')

try:
    opts, args = getopt.getopt(sys.argv[1:], 'h:p:b:f:')
    for o,a in opts:
        if o in ('-h'):
            host = a 
        if o in ('-p'):
            port = a 
        if o in ('-b'):
            block_size = int(a)
        if o in ('-f'):
            filename = a
except getopt.GetoptError:
    usage()
    sys.exit()

if ((host == None) or (port == None) or (filename == None)):
    usage()
    sys.exit()

def format_request(msg):
    request = 'writedisk' + ' '
    request += str(offset)
    request += ' '
    request += str(block_size)
    request += ' '
    request += '{0:08x}'.format(zlib.adler32(msg) & 0xffffffff)
    request += ' '
    request += base64.b64encode(msg)
    return (request)

def get_response():
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
    f = open(filename, 'rb')
except Exception, e:
    print ('unable to open ' + filename)
    sys.exit(1)

sckt = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sckt.settimeout(2)
sckt.connect((host, int(port)))

get_response()

blk = f.read(block_size)
while (blk):
    request = format_request(blk)
    print (request)
    sckt.send(request + '\r')
    response = get_response()
    print (response)
#    time.sleep(0.1)
    offset += block_size
    blk = f.read(block_size)
