import sys
import serial
import time
import datetime
#from random import randint

import msvcrt   # windows
#import kbhit    # linux

com_port = None

ports = ['COM5', 'COM4', 'COM3', 'COM2', 'COM1']   # windows
#ports = ['/dev/ttyUSB1']                           # linux

done = False

term_io_func_ptr = None

command = ''

pathname = None

status_line_num = 0
header_count = 0

auto_status_intvl_list_i = 5
auto_status_interval_list = [10, 20, 30, 60, 120, 180, 300, 600, 900]

auto_status_interval = auto_status_interval_list[auto_status_intvl_list_i]  # seconds

last_status_time = time.time()

header_cmnds   = '           TC    gd     gv    p398  p310  p316  p326  p330  p342  p346   a0    a1    a2    a3    a5    a6       E (3) E (1) E (2)  '
header_one     = '        cld hd  LNA    Vac   Turbo Turbo Turbo elec.  botm  brng motor board inlet  pax  exhst rjctn housn  Fan  crnt  max   min   '
header_two     = '  Time    (K)   (K)    mbar   RPM   amps watts  (C)   (C)   (C)   (C)   (C)   (C)   (C)   (C)   (C)   (C)   RPM watts watts watts  '
#                 HH:MM:SS 000.0 000.0 0.0e-00 00000 000.0 00000 00000 00000 00000 00000 000.0 000.0 000.0 000.0 000.0 000.0 0000 000.0 000.0 000.0 X

line_fields    = '{0:>8.8s} {1:>5.5s} {2:>5.5s} {3:>7.7s} {4:>5.5s} {5:>5.5s} {6:>5.5s} {7:>5.5s} {8:>5.5s} {9:>5.5s} {10:>5.5s} {11:>5.5s} {12:>5.5s} {13:>5.5s} {14:>5.5s} {15:>5.5s} {16:>5.5s} {17:>4.4s} {18:>5.5s} {19:>5.5s} {20:>5.5s} {21:>1.1s}'

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

def com_port_recv_line():
    line = ""

    timeout_count = 0

    while True:
        recv_char = com_port_recv_char()
        if (len(recv_char) == 0):
            timeout_count = timeout_count + 1
            if (not(timeout_count < 3)):
                line = ""
                break
            continue
        if (ord(recv_char) == 0x0d):
            break
        if ((ord(recv_char) >= 0x20) and (ord(recv_char) <= 0x7e)):
            line = line + recv_char
            timeout_count = 0

    return (line)

def auto_status_interval_desc():
    return ('Automatic status interval is {0:d} {1:s}.'.format( 
        auto_status_interval if (auto_status_interval < 60) else (auto_status_interval / 60),
        'seconds' if (auto_status_interval < 60) else ('minute(s)')))

def feed_status():
    global status_line_num
    global header_count
    global last_status_time

    if (not(status_line_num < 10)):
        status_line_num = 0

    if (len(command) > 0):
        sys.stdout.write('\r\n')
        sys.stdout.flush()       

    if (status_line_num == 0):
        sys.stdout.write(header_one if not(header_count & 1) else header_cmnds)
        sys.stdout.write('\r\n')
        sys.stdout.write(header_two)
        sys.stdout.write('\r\n')
        sys.stdout.flush()
        header_count = header_count + 1

    last_status_time = time.time()

    get_and_print_status()

    status_line_num = status_line_num + 1    

def main():
    global com_port
    global last_status_time 
    global pathname
    global command

    com_port = open_com_port()
    if (not(com_port)):
        sys.exit()

    dt = datetime.datetime.now()
    filename = dt.strftime('%Y-%m-%d-%H-%M-%S-antonio-feed-status.log')
    pathname = 'logs' + '\\' + filename  # windows
#    pathname = filename                  # linux

    print (auto_status_interval_desc())
    print ("Use '<' or '>' to decrease or increase status interval.")
    print ('Press ? to request and display feed status on demand.')
    print ('Status logged to {0:s}'.format(filename))

    f = open(pathname, 'at')
    f.write(header_one + '\n')
    f.write(header_two + '\n')
    f.close()

    term_io_func_ptr = term_io_handle_keycode

    com_port.write(chr(0x0d))
    time.sleep(0.3)
    com_port.write('minex')
    com_port.write(chr(0x0d))

    last_status_time = time.time()

#    kb = kbhit.KBHit()                     # linux

    while (not(done)):
#        time.sleep(1.0 / 1000.0)
        if (msvcrt.kbhit()):               # windows
            keycode = ord(msvcrt.getch())  # windows
            term_io_func_ptr(keycode)      # windows
#        if kb.kbhit():                     # linux
#             keycode = ord(kb.getch())     # linux
#             if (keycode == 0x0a):         # linux
#                 keycode = 0x0d            # linux
#             term_io_func_ptr(keycode)     # linux
        recv_data = com_port_recv_char()
        if (len(recv_data) > 0):
            sys.stdout.write(recv_data)        
            sys.stdout.flush()
        if ((time.time() - last_status_time) > float(auto_status_interval)):
            feed_status()
#            last_status_time = time.time()
            command = ''
            
    com_port.close()

#    kb.set_normal_term()                   # linux

def term_io_handle_keycode(keycode):
    global term_io_func_ptr
    global command

    if (keycode == 0x00):
        term_io_func_ptr = term_io_handle_extended_00_keycode
        return

    if (keycode == 0xe0):
        term_io_func_ptr = term_io_handle_extended_0e_keycode
        return

    if (keycode == 0x08):
        term_io_backspace()
        return

    if (keycode == 0x3f):
        term_io_get_status()
        return

    if (keycode == ord('<')):
        decrease_auto_status_interval()
        return

    if (keycode == ord('>')):
        increase_auto_status_interval()
        return

    if ((keycode >= 0x20) and (keycode <= 0x7e)):
        command = command + chr(keycode)
        com_port.write(chr(keycode))
        return

    if (keycode == 0x0d):
        term_io_cr()
        return

def term_io_handle_extended_00_keycode(keycode):
    global term_io_func_ptr

    term_io_func_ptr = term_io_handle_keycode

def term_io_handle_extended_0e_keycode(keycode):
    global term_io_func_ptr

    term_io_func_ptr = term_io_handle_keycode

def term_io_backspace():
    global command

    if (len(command) > 0):
        command = command[0:-1]

    com_port.write(chr(0x08))

def term_io_get_status():
    global command

    feed_status()

    command = ''

def term_io_cr():
    global command
    global status_line_num
    global done

    com_port.write(chr(0x0d))

    command = command.strip()
    if ((command.lower() == 'quit') or (command.lower() == 'exit')):
        done = True
        return 

    status_line_num = 0

    command = ''


def decrease_auto_status_interval():
    global auto_status_intvl_list_i
    global auto_status_interval
    global last_status_time
    global status_line_num

    if (auto_status_intvl_list_i > 0):
        auto_status_intvl_list_i = auto_status_intvl_list_i - 1
        auto_status_interval = auto_status_interval_list[auto_status_intvl_list_i]

    last_status_time = time.time()
    status_line_num = 0
    print (auto_status_interval_desc())

def increase_auto_status_interval():
    global auto_status_intvl_list_i
    global auto_status_interval
    global last_status_time
    global status_line_num

    if (auto_status_intvl_list_i < (len(auto_status_interval_list) - 1)):
        auto_status_intvl_list_i = auto_status_intvl_list_i + 1
        auto_status_interval = auto_status_interval_list[auto_status_intvl_list_i]

    last_status_time = time.time()
    status_line_num = 0
    print (auto_status_interval_desc())    

def get_and_print_status():
    while (len(com_port_recv_char()) == 1):
        continue

    dt = datetime.datetime.now()

    f = open(pathname, 'at')

    status_line = line_fields.format(
        dt.strftime('%H:%M:%S'),
        session('TC', 2, 1, cnvrt_temp),
        session('gd', 1, 1, cnvrt_temp),
        session('gv', 1, 1, cnvrt_vacuum),
        session('p398', 1, 1, cnvrt_turbo_rpm),
        session('p310', 1, 1, cnvrt_turbo_amps),
        session('p316', 1, 1, cnvrt_turbo_watts),
        session('p326', 1, 1, cnvrt_turbo_temp),
        session('p330', 1, 1, cnvrt_turbo_temp),
        session('p342', 1, 1, cnvrt_turbo_temp),
        session('p346', 1, 1, cnvrt_turbo_temp),
        session('gt a0', 1, 1, cnvrt_temp),
        session('gt a1', 1, 1, cnvrt_temp),
        session('gt a2', 1, 1, cnvrt_temp),
        session('gt a3', 1, 1, cnvrt_temp),
        session('gt a5', 1, 1, cnvrt_temp),
        session('gt a6', 1, 1, cnvrt_temp),
        session('getfanrpm', 1, 1, cnvrt_fan_rpm),
        session('E', 2, 3, cnvrt_E_pwr),
        session('E', 2, 3, cnvrt_E_max),
        session('E', 2, 3, cnvrt_E_min),
        session('getcryoattemp', 1, 1, cnvrt_at_temp)
    )

    sys.stdout.write(status_line)
    sys.stdout.write('\r\n')
    sys.stdout.flush()

    f.write(status_line)
    f.write('\n')
    f.close()

    while (len(com_port_recv_char()) == 1):
        continue

def session(command, num_echo_lns, num_rspns_lns, cnvrt_func):
    # try twice
    for r in range(2):
        try:
            while (len(com_port_recv_char()) == 1):
                continue
            lines = []
            com_port.write(command + '\r')
            for l in range(num_echo_lns):
                com_port_recv_line()  # throw away echoed command           
            # then get response line(s)
            for l in range(num_rspns_lns):
                lines.append('')
                lines[l] = com_port_recv_line()
                if (not(len(lines[l]) > 0)):
                    raise Exception('')
            return (cnvrt_func(lines))
        except Exception:
            time.sleep(0.1)
            pass

    return ('----------')  # will fill because wider than any particular field

def cnvrt_temp(lines):
    return ('{0:5.1f}'.format(float(lines[0])))

def cnvrt_vacuum(lines):
    return ('{0:.1e}'.format(float(lines[0])))

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
    
main()
