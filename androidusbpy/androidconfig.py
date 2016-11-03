"""
Creators: Pseudonymous
Date: November 1, 2016
License: GPLv3 (General Public License version 3)

File: androidconfig.py
Description: Asynchronous server/client to handle android accessory mode (Configuration file)

"""

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

