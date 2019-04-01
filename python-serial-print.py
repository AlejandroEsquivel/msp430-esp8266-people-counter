# University of British Columbia
# Department of Physics & Astronomy

#!/usr/bin/python2.7
import serial # for serial port
import numpy as np # for arrays, numerical processing

#define the serial port. Pick one:
port = "/dev/ttyACM0"  #for Linux

#start our program proper:
#open the serial port
try:
    ser = serial.Serial(port,115200,timeout = 1) 
# with timeout=0, read returns immediately, even if no data
except:
    print ("Opening serial port",port,"failed")
    print ("Edit program to point to the correct port.")
    print ("Hit enter to exit")
    raw_input()
    quit()

ser.flushInput()

print(ser);
while(1): #loop forever
    data = ser.readline() # look for a character from serial port - will wait for up to 50ms (specified above in timeout)
    if len(data) > 0: #was there a byte to read?
        print data
