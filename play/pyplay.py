#!/usr/bin/python

import socket
import sys
import struct
import time

port = int(sys.argv[1]);
print "port = %d\n " %  port
print " argcount = %d\n " %  len(sys.argv)
if (len(sys.argv)>2) :
	ipaddr = sys.argv[2]
	print "ipaddress = %s\n"  % ipaddr


