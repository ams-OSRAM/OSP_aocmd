#!/usr/bin/env python3
# cmdint - command interpreter library to send commands and receive responses over a serial port


import sys
import glob
import serial
import time
import datetime


class CmdIntException(Exception):
    def __init__(self,*args,**kwargs):
        Exception.__init__(self,*args,**kwargs)


   
class CmdInt:
    """Send commands over a serial port and captures response."""
    def __init__(self, port=None):
        """Creates an CmdInt instance. If port is passed, open() is called."""
        self.serial= None
        self.logfile= None
        if not port is None: self.open(port)
    def logstart(self,filename="__cmdint.log",filemode="w"):
        """Starts logging to file 'filename' (for append) of all commands and responses."""
        self.logfilename= filename
        self.logfile= open(filename,filemode)
        self.log('Log start '+filename)
    def logstop(self):
        if self.logfile!=None:
            """Stops logging."""
            self.log('Log stop')
            self.logfile.close()
            self.logfile= None
    def log(self,msg,tag="!"):
        """Add message 'msg' to the log, tagging the message with character 'tag'."""
        # Prepends time stamp and tag: '2016-12-08 13:40:20.879844 ! '
        # Make NLs visible by rendering them as "«"
        rep= "«\n                           {} ".format(tag) # Spaces are for indent
        msg= msg.replace("\n",rep)
        if msg.endswith(rep): msg= msg[0:len(msg)-len(rep)+1]
        self.logfile.write("{} {} {}\n".format(datetime.datetime.today(),tag,msg))    
    def open(self,port):
        """Opens serial port 'port' towards the dongle."""
        if self.logfile!=None: self.log('open port '+port)
        if self.serial!=None: raise CmdIntException("open(): COM port already open - Tip: either call e=Xxx();e.open(\"COM4\") or e=Xxx(\"COM4\").")
        self.serial= serial.Serial(port,baudrate=115200,timeout=0.01)
        self.rxbuf=b"" # Clear the receive buffer.
        res=self.exec("") # Clear any pending command.
        res=self.exec("echo disable","echo: echoing disabled\r\n>> ") # Explicitly sync.
    def close(self, nice=True):
        """Closes the serial port. When 'nice', first re-enables echoing."""
        if nice : res=self.exec("echo enable\n") # Switch echo back on
        if self.logfile!=None: self.log("close port")
        self.serial.close()
        self.serial= None
    def isopen(self):
        """Returns true if the serial port to the dongle is open."""
        return not self.serial is None
    def exec(self,icmd,isync=">> ", timeout_sec = 1.5 ):
        """Send command 'icmd'; waiting for response to have 'isync'. Returns response up to 'isync'."""
        cmd= icmd.encode(); sync= isync.encode() # Convert strings to bytes
        self.serial.write(cmd)
        self.serial.write(b"\n")
        if self.logfile!=None: self.log(icmd+"\n",">")
        pos,time0 = -1,time.time();
        while (pos<0) and (time.time()-time0<timeout_sec):
            self.rxbuf+= self.serial.read(1000)
            pos= self.rxbuf.find(sync)
        if pos<0 : 
            raise CmdIntException("exec(): sync not received ["+self.rxbuf.decode()+"]")
        if self.logfile!=None: self.log(self.rxbuf[:pos+len(sync)].decode(),"<")
        res=self.rxbuf[:pos]
        self.rxbuf=self.rxbuf[pos+len(sync):]
        return res.decode() # Convert bytes to strings


def cmdint_ports(Find=CmdInt,Ex=CmdIntException):
    """Returns an array of the serial ports that have a command interpreter. """
    """Pass a class name of a descender to check for a sensor chip."""
    """For Ex pass the exception the open command of the descender generates."""
    if sys.platform.startswith('win'):
        ports = ['COM%s' % (i + 1) for i in range(255)]
    elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
        # this excludes your current terminal "/dev/tty"
        ports = glob.glob('/dev/tty[A-Za-z]*')
    elif sys.platform.startswith('darwin'):
        ports = glob.glob('/dev/tty.*')
    else:
        raise EnvironmentError('Unsupported platform')
    result = []
    for port in ports:
        try:
            c= Find() # call the passed constructor
            c.open(port)
            c.close(True)
            result.append(port)
        except (OSError, serial.SerialException, CmdIntException, Ex):
            pass
    return result


if __name__ == "__main__":
    print("this script is not intended to be run alone.")
        
