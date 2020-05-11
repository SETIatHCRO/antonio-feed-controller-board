#!/usr/bin/env python
import sys
import getopt
import struct
import re
import serial
import zlib
import base64

SYSTEM = 'linux'

com_port = None

if (SYSTEM == 'windows'):
    ports = ['COM6', 'COM5', 'COM4', 'COM3', 'COM2', 'COM1']
if (SYSTEM == 'linux'):
    ports = [sys.argv[1]] 

if (SYSTEM == 'windows'):
    filename = sys.argv[1]
if (SYSTEM == 'linux'):
    filename = sys.argv[2] 

fd = None

BLOCK_SIZE = 512

def open_com_port():
    for port in ports:
        try:
            print ('open {0:s} ... '.format(port)),
            com_port = serial.Serial(port, 19200, timeout=0.1)
            print ('O.K.')
            return (com_port) 
        except Exception:  
            print ('fail')

    return (None)

def com_port_recv_char():
    return (com_port.read(1))        

def com_port_recv_line(*arg):
    line = ""

    timeout_N = 100

    timeout_count = 0

    if (len(arg) == 1):
        timeout_N = arg[0]

    while True:
        recv_char = com_port_recv_char()
        if (len(recv_char) == 0):
            timeout_count = timeout_count + 1
            if (not(timeout_count < timeout_N)):
                line = ""
                break
            continue
        if (ord(recv_char) == 0x0d):
            break
        if ((ord(recv_char) >= 0x20) and (ord(recv_char) <= 0x7e)):
            line = line + recv_char
            timeout_count = 0

    return (line)

def update_failure():
    raw_input("feed control board update FAILED - press any key to exit")
    sys.exit(1)

def update_success():
    raw_input("feed control board update successful - press any key to exit")
    sys.exit(0)

def main():
    global com_port
    global fd

    com_port = open_com_port()
    if (not(com_port)):
        update_failure()

    print("got port")
    try:
        fd = open(filename, 'rb')
    except Exception, e:
        print ('unable to open disk image ' + filename)
        update_failure()
    print('got file')

    com_port.write(chr(0x0d))
    com_port_recv_line()
    com_port.write(chr(0x0d))
    com_port_recv_line()

    cmnd = 'bootloader'
    com_port.write(cmnd)
    com_port.write(chr(0x0d))
    print (cmnd)
    rspns = com_port_recv_line()
    print(rspns)
    print ("done")

    if not ((re.search(r'Bootloader', rspns)) or (re.search(r'bootloader',rspns))):
        print('resetting')
        com_port_recv_line()
        cmnd = 'reset'
        com_port.write(cmnd)
        com_port.write(chr(0x0d))
        print (cmnd)
        rspns = com_port_recv_line()
        print (rspns)
        if (not (re.search(r'Bootloader', rspns) or re.search(r'bootloader',rspns)) ):
            update_failure()
    
    print("writing disk")
    com_port_recv_line()
    if (not(writedisk())):
        update_failure()

    print("erase flash")
    com_port_recv_line()
    cmnd = 'eraseflash'
    com_port.write(cmnd)
    com_port.write(chr(0x0d))
    print (cmnd)
    rspns = com_port_recv_line(70)
    print (rspns)

    if (not(re.search(r'OK', rspns))):
        update_failure()

    com_port_recv_line()
    cmnd = 'programflash ANTONI~1.HEX'
    com_port.write(cmnd)
    com_port.write(chr(0x0d))
    print (cmnd)
    rspnsln1 = com_port_recv_line(70)
    print (rspnsln1)
    rspnsln2 = com_port_recv_line()
    print (rspnsln2)

    if (not(re.search(r'records', rspnsln1))):
        update_failure()
    if (not(re.search(r'OK', rspnsln2))):
        update_failure()

    com_port_recv_line()
    cmnd = 'jumptoapp'
    com_port.write(cmnd)
    com_port.write(chr(0x0d))
    print (cmnd)
    rspns = com_port_recv_line(100)
    print (rspns)

    if (not(re.search(r'Monitor and Control Firmware', rspns))):
        update_failure()

    update_success()

def format_request(blk, offset):
    request = 'writedisk' + ' '
    request += str(offset)
    request += ' '
    request += str(BLOCK_SIZE)
    request += ' '
    request += '{0:08x}'.format(zlib.adler32(blk) & 0xffffffff)
    request += ' '
    request += base64.b64encode(blk)
    return (request)

def writedisk():
    offset = 0

    blk = fd.read(BLOCK_SIZE)
    if (not(blk)):
        return (False)

    magic_num = struct.unpack('<510x1H', blk)
    if (not(magic_num[0] == 0xaa55)):
        print ('MBR not found in ' + filename)
        return (False)

    retry_count = 0

    while (blk):
        request = format_request(blk, offset)
        print (request[0:54] + ' ... ' + request[-16:] + ' ')
        com_port.write(request)
        com_port.write(chr(0x0d))
        rspns = com_port_recv_line()
        print (rspns)
        if (re.search(r'ACK', rspns)):
            retry_count = 0
        else:
            com_port_recv_line()
            retry_count += 1
        if (retry_count == 1):
            continue
        if (retry_count > 2):
            return (False)
        offset += BLOCK_SIZE
        blk = fd.read(BLOCK_SIZE)

    return (True)

main()

