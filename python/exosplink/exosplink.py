import time
from libosplink.cmdint import cmdint_ports
from libosplink.osplink import OSPlink,OSPlinkException

def main():
    print("Scanning for ports with OSPlink... ")
    ports= cmdint_ports(OSPlink,OSPlinkException)
    print("  Found",ports)
    if len(ports)==0: print("  No port with OFS found"); return
    port=ports[0]
    print("  Opening",port)
    osplink=OSPlink(port)
    print()
    
    #osplink.boardreboot()
    #print("Version")
    #v= osplink.version()
    #print(f"  {v}")

    osplink.logstart()

    dirmux,last= osplink.osp_resetinit()
    print( f"resetinit() -> dirmux={dirmux} last={last}" )

    osplink.osp_clrerror(0x000)
    print("clrerror(0x000)")

    osplink.osp_goactive(0x000)
    print("goactive(0x000)")

    for i in range(5):
        print( f"round {i}")
        osplink.osp_setpwmchn(0x001,0,0x3333,0x0000,0x0000)
        print( " setpwmchn(0x001,red)" )
        time.sleep(0.1)
        osplink.osp_setpwmchn(0x001,0,0x0000,0x3333,0x0000)
        print( " setpwmchn(0x001,grn)" )
        time.sleep(0.1)
        osplink.osp_setpwmchn(0x001,0,0x0000,0x0000,0x3333)
        print( " setpwmchn(0x001,blue)" )
        time.sleep(0.1)
    
    osplink.close()

if __name__ == "__main__":
  main()


