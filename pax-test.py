import sys
import msvcrt
import time
import os
import select
import socket
import re

command = ""

terminal_io_func_ptr = None

PAX_HOST = "192.168.243.2"
PAX_PORT = 23

DEFAULT_GET_STATUS_SLEEP = 0.7

pax_socket = None

re_obj_set_bias = re.compile( 
    r"(setvm|setvg|setvd) *(x|y) *(-{0,1}[0-9]*([.][0-9]*)*).*")

auto_command = None
auto_command_pol = None
auto_command_value = 0.0

status_line_num = 0

done = False;


def terminal_io_backspace():
    global command

    if (len(command) > 0):
        command = command[0:-1]
    sys.stdout.write("\r" + command + " " +
        "\r" + command)

def terminal_io_handle_extended_00_keycode(keycode):
    global terminal_io_func_ptr

    terminal_io_func_ptr = terminal_io_handle_keycode

def terminal_io_handle_extended_E0_keycode(keycode):
    global terminal_io_func_ptr

    terminal_io_func_ptr = terminal_io_handle_keycode

    delta = 0
    if (keycode == 72):
        delta = 1
    if (keycode == 80):
        delta = -1
    if (delta <> 0):
        if (auto_command):
            pax_send_auto_command(delta)
            pax_get_status(DEFAULT_GET_STATUS_SLEEP)

def terminal_io_handle_keycode(keycode):
    global terminal_io_func_ptr
    global command

    if (keycode == 0x00):
        terminal_io_func_ptr = \
            terminal_io_handle_extended_00_keycode
        return
    if (keycode == 0xE0):
        terminal_io_func_ptr = \
            terminal_io_handle_extended_E0_keycode
        return
    if (keycode == 0x08):
        terminal_io_backspace()
        return
    if (keycode == 0x3F):
        terminal_io_get_status()
        return
    if ((keycode >= 0x20) and (keycode <= 0x7E)):
        command = command + chr(keycode)
        sys.stdout.write(chr(keycode))
        return
    if (keycode == 0x0D):
        terminal_io_cr()
        return

def terminal_io_get_status():
    pax_get_status(0.2)
    command = ""

def terminal_io_cr():
    global command
    global done
    global status_line_num

    command = command.strip()
    if ((command.lower() == "quit") or (command.lower() == "exit")): 
        done = True
        return

    sys.stdout.write("\r\n")
    pax_socket_send(command)

    if (match_on_set_bias_command()):
        sys.stdout.write(pax_socket_receive_line())
        status_line_num = 0
        pax_get_status(DEFAULT_GET_STATUS_SLEEP)

    command = ""

def match_on_set_bias_command():
    global auto_command, auto_command_pol, auto_command_value

    match_obj = re_obj_set_bias.match(command) 
    if (match_obj):
        auto_command = match_obj.group(1)
        auto_command_pol = match_obj.group(2)
        auto_command_value = float(match_obj.group(3)) 
        return (True)

    return (False)

def main():
    global terminal_io_func_ptr

    establish_route()
    pax_socket_open()
    sys.stdout.write("\r\n")
    terminal_io_func_ptr = terminal_io_handle_keycode
    while (not(done)):
        if msvcrt.kbhit():
            keycode = ord(msvcrt.getch())
            terminal_io_func_ptr(keycode)
        else:
            sys.stdout.write(pax_socket_receive()) 
    pax_socket_close()

def establish_route():
    route_command = "ROUTE ADD 192.168.243.2 169.254.0.1"
    sys.stdout.write(route_command + "\r\n")
    os.system(route_command) 
    time.sleep(0.5)

def pax_socket_open():
    global pax_socket

    if (not(pax_socket)):
        pax_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        pax_socket.settimeout(1.0)
        pax_socket.connect((PAX_HOST, PAX_PORT))

def pax_socket_close():
    global pax_socket

    if (pax_socket):
        pax_socket.close()
        pax_socket = None

def pax_socket_send(message):
    if (pax_socket):
        pax_socket.send(message + "\r")

def pax_socket_receive():
    if (pax_socket):
        ready_to_read, ready_to_write, in_error = \
            select.select([pax_socket], [], [], 0.1)
        if (len(ready_to_read) > 0):
            return(pax_socket.recv(9999))
    else:
        time.sleep(0.1)

    return ("")

def pax_socket_receive_line():
    line = ""

    for i in range(5):
        line = line + pax_socket_receive()
        if (len(line) > 0):
            if (ord(line[-1]) == 0x0A):
                return (line)

    return (line)

def pax_send_auto_command(delta):
    global auto_command_value

    if (auto_command == "setvm"):
        delta = delta * 0.1
    if (auto_command == "setvg"):
        delta = delta * 0.02
    if (auto_command == "setvd"):
        delta = delta * 0.1

    auto_command_value = auto_command_value + delta

    if (auto_command == "setvm"):
        if (auto_command_value < -1.8):
            auto_command_value = -1.8
        if (auto_command_value > 0.0):
            auto_command_value = 0.0

    if (auto_command == "setvg"):
        if (auto_command_value < -1.5):
            auto_command_value = -1.5
        if (auto_command_value > 2.2):
            auto_command_value = 2.2 

    if (auto_command == "setvd"):
        if (auto_command_value < 0.0):
            auto_command_value = 0.0
        if (auto_command_value > 2.3):
            auto_command_value = 2.3 

    auto_command_message = "%s %s %5.2f" % \
        (auto_command, auto_command_pol, auto_command_value)
    pax_socket_send(auto_command_message + "\r")
    response = pax_socket_receive_line()


def pax_get_status(sleep_seconds):
    global status_line_num

    vmx = 0.0 ; vmy = 0.0
    vgx = 0.0 ; vgy = 0.0
    vdx = 0.0 ; vdy = 0.0
    idx = 0.0 ; idy = 0.0

    time.sleep(sleep_seconds)

    pax_socket_send("getvm x")
    response = pax_socket_receive_line()
    try:
        vmx = float(response)
    except:
        sys.stdout.write(response.strip() + "\r\n") 
        return

    pax_socket_send("getvg x")
    response = pax_socket_receive_line()
    vgx = float(response)

    pax_socket_send("getvd x")
    response = pax_socket_receive_line()
    vdx = float(response)

    pax_socket_send("getid x")
    response = pax_socket_receive_line()
    idx = float(response)

    pax_socket_send("getvm y")
    response = pax_socket_receive_line()
    vmy = float(response)

    pax_socket_send("getvg y")
    response = pax_socket_receive_line()
    vgy = float(response)

    pax_socket_send("getvd y")
    response = pax_socket_receive_line()
    vdy = float(response)

    pax_socket_send("getid y")
    response = pax_socket_receive_line()
    idy = float(response)

    header = "   command     " + \
        "   VmX   VgX   VdX   IdX  " + "   VmY   VgY   VdY   IdY"
    if ((status_line_num % 10) == 0):
        sys.stdout.write(header + "\r\n")
    status_line_num = status_line_num + 1 

    status_line_format = "%5s %1s %5.2f | " + \
        "%5.2f %5.2f %5.2f %5.2f | %5.2f %5.2f %5.2f %5.2f"
    status_line = status_line_format % \
        (auto_command, auto_command_pol, auto_command_value, \
         vmx, vgx, vdx, idx, vmy, vgy ,vdy, idy)
    sys.stdout.write(status_line + "\r\n")

main()

