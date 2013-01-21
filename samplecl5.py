#!/usr/bin/python
import socket
import sys
import struct
import time

def new_sock():
  global port
  sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  sock.settimeout(0.200)
  time.sleep(0.05)
  sock.connect(('localhost', port))
  return sock

port = int(sys.argv[1])


# (5)
sock = new_sock()
time.sleep(0.001)
v = [-2]*4096
try:
  sock.sendall("uptime"*4094 + "load" + "uptime")
  r = []
  for i in range(4096):
    r += [sock.recv(4)]
  if len(r) == 4096:
    v = map(lambda e: struct.unpack("i", e)[0], r)
except:
  pass
if len(v) < 4096 or -2 in v or v[4094] != 1:
  print "TEST 2.5: ERROR... Buffer overrun test (or maybe load) failed"
else:
  print "TEST 2.5: OK... Received all the correct return codes"

