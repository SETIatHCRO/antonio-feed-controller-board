import getopt
import sys
import time
import datetime
import socket
import select
#import threading

host = '127.0.0.1'
port = 1518

line_fields = '{0:>8.8s} {1:>5.5s} {2:>5.5s} {3:>7.7s} {4:>5.5s} {5:>5.5s} {6:>5.5s} {7:>5.5s} {8:>5.5s} {9:>5.5s} {10:>5.5s} {11:>5.5s} {12:>5.5s} {13:>5.5s} {14:>5.5s} {15:>5.5s} {16:>5.5s} {17:>4.4s} {18:>5.5s} {19:>5.5s} {20:>5.5s} {21:>1.1s}'

response = ''

def usage ():
    print('python feedmonitor.py -h host -p port')

def filter_chars(chars):
    filtered_string = ''

    for i in range(len(chars)):
        char = chars[i]
        if ((char == '|') or (ord(char) == 0x0d) or (ord(char) == 0x0a)):
            filtered_string = filtered_string + chr(0x0d)
            continue
        if ((ord(char) >= 0x20) and (ord(char) <= 0x7e)):
            filtered_string = filtered_string + char

    return (filtered_string)

def get_line(sckt):
    global response

    timeout_count = 0
    while (timeout_count < 9):
        lines = response.split(chr(0x0d), 1)
        if (len(lines) == 2):
            response = lines[1]
            return (lines[0])
        readable, writable, exceptional = select.select([sckt], [], [], 0.1)
        if (readable):
            response = response + filter_chars(sckt.recv(999))
            timeout_count = 0
        else:
            timeout_count = timeout_count + 1

    return (None)  # timeout

def purge_response(sckt):
    global response

    response = ''

    timeout_count = 0
    while (timeout_count < 2):
        readable, writable, exceptional = select.select([sckt], [], [], 0.1)
        if (readable):
            sckt.recv(999)
            timeout_count = 0
        else:
            timeout_count = timeout_count + 1            

def get_and_print_status(sckt):
    dt = datetime.datetime.now()

    status_line = line_fields.format(
        dt.strftime('%H:%M:%S'),
        session(sckt, 'TC', 1, 1, cnvrt_temp),
        session(sckt, 'gd', 0, 1, cnvrt_temp),
        session(sckt, 'gv', 0, 1, cnvrt_vacuum),
        session(sckt, 'p398', 0, 1, cnvrt_turbo_rpm),
        session(sckt, 'p310', 0, 1, cnvrt_turbo_amps),
        session(sckt, 'p316', 0, 1, cnvrt_turbo_watts),
        session(sckt, 'p326', 0, 1, cnvrt_turbo_temp),
        session(sckt, 'p330', 0, 1, cnvrt_turbo_temp),
        session(sckt, 'p342', 0, 1, cnvrt_turbo_temp),
        session(sckt, 'p346', 0, 1, cnvrt_turbo_temp),
#        session(sckt, 'gt a0', 0, 1, cnvrt_temp),
        session(sckt, 'gettemp a0', 0, 1, cnvrt_temp),
#        session(sckt, 'gt a1', 0, 1, cnvrt_temp),
        session(sckt, 'gettemp a1', 0, 1, cnvrt_temp),
        session(sckt, 'gt a2', 0, 1, cnvrt_temp),
        session(sckt, 'gt a3', 0, 1, cnvrt_temp),
        session(sckt, 'gt a5', 0, 1, cnvrt_temp),
        session(sckt, 'gt a6', 0, 1, cnvrt_temp),
        session(sckt, 'getfanrpm', 0, 1, cnvrt_fan_rpm),
        session(sckt, 'E', 1, 3, cnvrt_E_pwr),
        session(sckt, 'E', 1, 3, cnvrt_E_max),
        session(sckt, 'E', 1, 3, cnvrt_E_min),
        session(sckt, 'getcryoattemp', 0, 1, cnvrt_at_temp)
    )

    print(status_line)

def session(sckt, command, num_echo_lns, num_rspns_lns, cnvrt_func):
    for r in range(2):
        try:
            purge_response(sckt)
            lines = []
            sckt.send(command + '\r')
            for l in range(num_echo_lns):
                get_line(sckt)  # throw away echoed command           
            # then get response line(s)
            for l in range(num_rspns_lns):
                lines.append('')
                ln = get_line(sckt)
                if (ln is None):
                    raise Exception('')
                lines[l] = ln
                if (not(len(lines[l]) > 0)):
                    raise Exception('')
            return (cnvrt_func(lines))
        except Exception:
            time.sleep(0.7)
            pass

    return ('----------')  # will fill because wider than any particular field

def cnvrt_temp(lines):
    return ('{0:5.1f}'.format(float(lines[0])))

def cnvrt_vacuum(lines):
    return ('{0:.1e}'.format(float(lines[0].split()[0])))

def cnvrt_turbo_rpm(lines):
    return ('{0:5d}'.format(int(lines[0], 10)))

def cnvrt_turbo_amps(lines):
    return ('{0:5.1f}'.format(int(lines[0], 10) / 100.0))

def cnvrt_turbo_watts(lines):
    return ('{0:5d}'.format(int(lines[0], 10)))

def cnvrt_turbo_temp(lines):
    return ('{0:5d}'.format(int(lines[0], 10)))

def cnvrt_fan_rpm(lines):
    return ('{0:4d}'.format(int(lines[0], 10)))

def cnvrt_E_pwr(lines):
    return ('{0:5.1f}'.format(float(lines[2])))

def cnvrt_E_min(lines):
    return ('{0:5.1f}'.format(float(lines[1])))

def cnvrt_E_max(lines):
    return ('{0:5.1f}'.format(float(lines[0])))

def cnvrt_at_temp(lines):
    if (lines[0].strip().startswith('y')):
        return ('Y')
    if (lines[0].strip().startswith('n')):
        return ('N')
    return (' ')

def main():
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

    sckt = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    try:
        sckt.settimeout(3.0)
        sckt.connect((host, int(port)))
    except:
        print('exception while attempting connection to host ' + host + ' on port ' + port)
        sys.exit()

    time.sleep(0.3)

    sckt.settimeout(None)
    sckt.setblocking(1)

    purge_response(sckt)
    sckt.send(chr(0x0d))
    get_line(sckt);

    purge_response(sckt)
    sckt.send(chr(0x0d))
    get_line(sckt);

#   try three times
    for i in range(3):
        purge_response(sckt)
        sckt.send('stty')
        sckt.send(chr(0x0d))
        ln = get_line(sckt);
        if (not(ln is None) and (ln == 'OK')):
            get_and_print_status(sckt)
            break
        time.sleep(0.3)

    sckt.close()

main()

