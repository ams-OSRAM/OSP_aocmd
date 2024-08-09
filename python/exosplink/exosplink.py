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

    print("resetinit")
    dirmux,last= osplink.osp_resetinit()
    print(f"  {dirmux}")
    print(f"  {last}")

    print("clrerror")
    osplink.osp_clrerror()

    print("goactive")
    osplink.osp_goactive()

    for i in range(100):
        print("setpwmchn(red)")
        osplink.osp_setpwmchn(0x001,0,0x3333,0x0000,0x0000)
        time.sleep(0.1)
        print("setpwmchn(grn)")
        osplink.osp_setpwmchn(0x001,0,0x0000,0x3333,0x0000)
        time.sleep(0.1)
        print("setpwmchn(blue)")
        osplink.osp_setpwmchn(0x001,0,0x0000,0x0000,0x3333)
        time.sleep(0.1)
    
    osplink.close()

if __name__ == "__main__":
  main()


