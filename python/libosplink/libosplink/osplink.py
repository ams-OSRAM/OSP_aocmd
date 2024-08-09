#!/usr/bin/env python3
# osplink.py - Library for the osplink command handler


import re
from time import sleep
from serial import SerialException
from libosplink.cmdint import CmdIntException, CmdInt, cmdint_ports


class OSPlinkException(Exception):
    def __init__(self,*args,**kwargs):
        Exception.__init__(self,*args,**kwargs)


class OSPlink(CmdInt):
    def open(self,port):
        """Opens serial port 'port' towards OSPlink."""
        super(OSPlink,self).open(port)
        long_appname = self.version("%La")
        if long_appname != "OSPlink" : raise OSPlinkException(f"Wrong firmware (expected 'OSPlink', is '{long_appname}')")
    def version(self,fmt="%La %Va"):
        """Returns the version; the 'fmt' can have % followed by a char (v,a,La,Va,r,c,i,t); these are replaced by version components.""" 
        v= self.exec("version")
        ga= re.search(r"app     : ((.*) (.*))\n",v)
        gr= re.search(r"runtime : (.*)\n",v)
        gc= re.search(r"compiler: (.*)\n",v);
        gi= re.search(r"arduino : (.*)\n",v);
        gt= re.search(r"compiled: (.*)\n",v);
        fmt= fmt.replace("%v",v)
        fmt= fmt.replace("%a",ga.group(1))         # a ='OSPlink 1.1'                     # all
        fmt=   fmt.replace("%La",ga.group(2))      # La='OSPlink'                         # long
        fmt=   fmt.replace("%Va",ga.group(3))      # Va='1.1'                             # version
        fmt= fmt.replace("%r",gr.group(1))         # r ='Arduino ESP32 2_0_14'
        fmt= fmt.replace("%c",gc.group(1))         # c ='8.4.0'        
        fmt= fmt.replace("%i",gi.group(1))         # i ='10816'        
        fmt= fmt.replace("%t",gt.group(1))         # t ='Nov  2 2022, 14:01:11'        
        return fmt
    def boardreboot(self):
        self.exec(f"board reboot")
        self.exec(f"echo disabled") # this disables echo again AND has as side effect an rxbuf sync
    def osp_resetinit(self):
        res = self.exec(f"osp resetinit")
        found = re.search(r"resetinit: (.*) (.*) \((.*)\)",res);
        if found.group(3)!="ok" : raise OSPlinkException(f"resetinit failed {found.group(3)}")
        return found.group(1), int(found.group(2),16)
    def osp_clrerror(self) :
        res = self.exec(f"@osp send 0 clrerror")
        found = re.search(r"rx none (.*)",res);
        if found.group(1)!="ok" : raise OSPlinkException(f"clrerror failed {found.group(1)}")
    def osp_goactive(self) :
        res = self.exec(f"@osp send 0 goactive")
        found = re.search(r"rx none (.*)",res);
        if found.group(1)!="ok" : raise OSPlinkException(f"goactive failed {found.group(1)}")
    def osp_setpwmchn(self,addr,chn,red,grn,blu) :
        res = self.exec(f"@osp send {addr:X} setpwmchn {chn:X} ff {red//256:X} {red%256:X} {grn//256:X} {grn%256:X} {blu//256:X} {blu%256:X}")
        found = re.search(r"rx none (.*)",res);
        if found.group(1)!="ok" : raise OSPlinkException(f"setpwmchn failed {found.group(1)}")


if __name__ == "__main__":
    print("this script is not intended to be run alone.")

