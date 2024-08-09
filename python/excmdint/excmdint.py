from libosplink.cmdint import CmdInt,cmdint_ports

def test(cmdint,cmd) :
    print( f"exec('{cmd}')")
    res = cmdint.exec(cmd)
    print( "->",res,end="")
    print()

def main():
    print("scanning for COM ports...")
    ports= cmdint_ports()
    print("found",ports)
    if len(ports)==0: print("no command interpreter found"); return
    port=ports[0]
    print("opening",port)
    cmdint=CmdInt()
    cmdint.open(port)
    print()
    
    test(cmdint,"echo line Hello, world!")
    test(cmdint,"echo faults")
    
    print(f"logging")
    cmdint.logstart()
    res=cmdint.exec("help echo")
    # print(res)
    cmdint.logstop()
    print(f"see {cmdint.logfilename}")

    cmdint.close()

if __name__ == "__main__":
  main()
