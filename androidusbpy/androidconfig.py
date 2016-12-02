"""
Creators: Pseudonymous
Date: November 1, 2016
License: GPLv3 (General Public License version 3)

File: androidconfig.py
Description: Asynchronous server/client to handle android accessory mode (Configuration file)

"""
from os.path import dirname

MANUFACTURE = "Pseudonymous"
MODEL = "Slumber"
DESCRIPTION = "Home automation hub service"
VERSION = "1.0.0-alpha"
VERSION_CONTROLLER = 2
URL = "http://pseudonymous.tk"
SERIAL = "1865"
ERROR_JSON = {"exec": "test"}
READY_JSON = {"state": True}

# Advanced Settings
ACCESSORY_VID = 0x18D1
ACCESSORY_PID = (0x2D00, 0x2D01, 0x2D04, 0x2D05)

NETLINK_KOBJECT_UEVENT = 15
TIMEOUT = 0

# Tcp Advanced
SIZEBUFFER = 28  # Size buffer to initially send to the app
TCP_BIND = "127.0.0.1"  # TCP bind address (Only localhost)
TCP_PORT = 3005  # TCP Server port
TCP_BUFF = 10485760  # TCP Connection buffer
TCP_CONN = 1  # Max connections
TRY_ACCESS = 5

'''
import serial
from tkinter import *
import threading
'''

# root = Tk()  # Create a background window


# Create a list
"""
def looper():
    global root
    ser = serial.Serial("/dev/ttyACM0", 115200)

    while 1:
        red = str(ser.readline())
        if "R" in red and "G" in red and "B" in red and "b" in red and "a" in red:
            try:
                R = int(red[red.index("<R>") + 3: red.index("<G>")])
                G = int(red[red.index("<G>") + 3: red.index("<B>")])
                B = int(red[red.index("<B>") + 3: red.index("<b>")])

                print("GOT: R: %d G: %d B: %d" % (R, G, B))
                mycolor = '#%02x%02x%02x' % (R, G, B)
                root.configure(bg=mycolor)

            except Exception as err:
                print("ERR %s" % str(err))


threading.Thread(target=looper).start()
root.mainloop()
"""
