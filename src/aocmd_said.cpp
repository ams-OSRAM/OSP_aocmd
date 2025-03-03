// aocmd_said.h - command handler for the "said" command - to send and receive said specific telegrams
/*****************************************************************************
 * Copyright 2024 by ams OSRAM AG                                            *
 * All rights are reserved.                                                  *
 *                                                                           *
 * IMPORTANT - PLEASE READ CAREFULLY BEFORE COPYING, INSTALLING OR USING     *
 * THE SOFTWARE.                                                             *
 *                                                                           *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       *
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         *
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS         *
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT  *
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,     *
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT          *
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     *
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY     *
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT       *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE     *
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.      *
 *****************************************************************************/


#include <Arduino.h>        // Serial.printf
#include <aoosp.h>          // aoosp_crc()
#include <aocmd_cint.h>     // aocmd_cint_register, aocmd_cint_isprefix, ...
#include <aocmd_said.h>     // own


// Prints results of a scan of an I2C bus (for single SAID)
static int aocmd_said_i2c_scan_uni(uint16_t addr, int verbose) {
  // Scan all i2c devices
  if( verbose ) Serial.printf("SAID %03X has I2C (now powered)\n",addr);
  int count= 0;
  for( uint8_t daddr7=0; daddr7<0x80; daddr7++ ) {
    if( verbose ) if( daddr7 % 16 == 0) Serial.printf("  %02x: ",daddr7);
    // Try to read at address 0 of device daddr7
    uint8_t buf[8];
    aoresult_t result = aoosp_exec_i2cread8(addr, daddr7, 0x00, buf, 1);
    int i2cfail=  result==aoresult_dev_i2cnack || result==aoresult_dev_i2ctimeout;
    if( result!=aoresult_ok && !i2cfail ) { Serial.printf("ERROR: aoosp_exec_i2cread8(%03X) failed (%s)\n", addr, aoresult_to_str(result) ); return 0; }
    if( i2cfail ) { if(verbose) Serial.printf(" %02x ",daddr7); } else Serial.printf("[%02x]",daddr7); // [] brackets indicate presence
    if( !i2cfail ) count++;
    if( verbose ) if( daddr7 % 16 == 15) Serial.printf("\n");
  }
  if( !verbose && count>0 ) Serial.printf(" ");
  Serial.printf("SAID %03X has %d I2C devices\n",addr, count);
  return count;
}


// Prints I2C bus scan result for "broadcast" (loops over chain)
static void aocmd_said_i2c_scan_broad(int verbose) {
  int i2ccount=0;
  int saidcount=0;
  for( int addr=AOOSP_ADDR_UNICASTMIN; addr<=aoosp_exec_resetinit_last(); addr++ ) {
    if( aoosp_exec_i2cpower(addr)==aoresult_ok ) {
      i2ccount+= aocmd_said_i2c_scan_uni(addr,verbose);
      saidcount++;
      if( verbose ) Serial.printf("\n");
    }
  }
  Serial.printf("total %d SAIDs have %d I2C devices\n", saidcount, i2ccount);

}


// Shows the current I2C bus frequency
static void aocmd_said_i2c_freq_show(uint16_t addr) {
  uint8_t flags;
  uint8_t speed;
  aoresult_t result= aoosp_send_readi2ccfg(addr, &flags, &speed);
  if( result!=aoresult_ok ) { Serial.printf("ERROR: readi2ccfg(%03X) failed (%s)\n", addr, aoresult_to_str(result) ); return; }
  Serial.printf("said(%03X).i2c.freq %d Hz (speed %d)\n", addr, aoosp_prt_i2ccfg_speed(speed), speed );
}


// Updates the current I2C bus frequency
static aoresult_t aocmd_said_i2c_freq_set(uint16_t addr, int speed) {
  uint8_t flags;
  uint8_t oldspeed;
  aoresult_t result;
  // read old flags
  result= aoosp_send_readi2ccfg(addr, &flags, &oldspeed);
  if( result!=aoresult_ok ) { Serial.printf("ERROR: readi2ccfg(%03X) failed (%s)\n", addr, aoresult_to_str(result) ); return result; }
  // write flags and new speed
  result= aoosp_send_seti2ccfg(addr, flags, speed);
  if( result!=aoresult_ok ) { Serial.printf("ERROR: seti2ccfg(%03X) failed (%s)\n", addr, aoresult_to_str(result) ); return result; }
  return aoresult_ok;
}


// Command handler for 'said i2c <addr> freq [<freq>]'
static void aocmd_said_i2c_freq(int argc, char * argv[], uint16_t addr ) {
  // Read freq?
  if( argc==4 ) { aocmd_said_i2c_freq_show(addr); return; }
  if( argc>5 ) { Serial.printf("ERROR: 'freq' has too many args\n"); return; }
  // Write freq, get the Hz
  int freq;
  if( !aocmd_cint_parse_dec(argv[4],&freq) ) { Serial.printf("ERROR: 'freq' expects <freq>, not '%s'\n",argv[4]); return; }
  // Convert freq to speed (hw speed code)
  int speed=AOOSP_I2CCFG_SPEED_MAX;
  while( speed!=AOOSP_I2CCFG_SPEED_MIN && freq<aoosp_prt_i2ccfg_speed(speed) ) {
    speed++;
  }
  // Write
  aoresult_t result=aocmd_said_i2c_freq_set(addr,speed);
  if( result!=aoresult_ok ) return;
  // Feedback
  if( argv[0][0]!='@' ) aocmd_said_i2c_freq_show(addr);
}


// Command handler for 'said i2c <addr> write <daddr7> <raddr> <data>...'
static void aocmd_said_i2c_write(int argc, char * argv[], uint16_t addr ) {
  #define WBUFSIZE 8
  uint8_t buf[WBUFSIZE];
  int argix= 4;
  int bufix= 0;
  while( argix<argc ) {
    uint16_t byte;
    if( !aocmd_cint_parse_hex(argv[argix],&byte) || byte > 255 ) { Serial.printf("ERROR: 'write' expects 00..FF, not '%s'\n",argv[argix]); return; }
    buf[bufix]= byte;
    argix++;
    bufix++;
    if( bufix>WBUFSIZE ) { Serial.printf("ERROR: 'write' has too many args\n"); return; }
  }
  // Checks
  if( bufix==0 || bufix==1 ) { Serial.printf("ERROR: 'write' expects <daddr7> and <raddr>\n"); return; }
  if( buf[0] & ~ 0x7F ) { Serial.printf("ERROR: 'write' expects <daddr7> to be 00..7F, not %02X\n",buf[0]); return; }
  int count= bufix-2;
  if( count!=1 && count!=2 && count!=4 && count!=6 ) { Serial.printf("ERROR: 'write' payload can only be 1, 2, 4, or 6 bytes (not %d)\n",count); return; }
  // Now write
  aoresult_t result= aoosp_exec_i2cwrite8(addr, buf[0], buf[1], buf+2, count);
  // Feedback
  if( result!=aoresult_ok ) { Serial.printf("ERROR: write(%03X) failed (%s)\n", addr, aoresult_to_str(result) ); return; }
  if( argv[0][0]!='@' ) Serial.printf("said(%03X).i2c.dev(%02X).reg(%02X) %s\n",addr,buf[0],buf[1], aoosp_prt_bytes(buf+2,count) );
}


// Command handler for 'said i2c <addr> read <daddr7> <raddr> <count>'
static void aocmd_said_i2c_read(int argc, char * argv[], uint16_t addr ) {
  // <daddr7>
  if( argc<5 ) { Serial.printf("ERROR: 'read' expects <daddr7>\n"); return; }
  uint16_t daddr7;
  if( !aocmd_cint_parse_hex(argv[4],&daddr7) || daddr7>0x7F ) { Serial.printf("ERROR: 'read' expects <daddr7> 00..7F, not '%s'\n",argv[4]); return; }
  // <raddr>
  if( argc<6 ) { Serial.printf("ERROR: 'read' expects <raddr>\n"); return; }
  uint16_t raddr;
  if( !aocmd_cint_parse_hex(argv[5],&raddr) || raddr>0xFF ) { Serial.printf("ERROR: 'read' expects <raddr> 00..FF, not '%s'\n",argv[5]); return; }
  // <count>
  uint16_t count;
  if( argc==6 ) {
    count= 1;
  } else if( argc==7 ) {
    if( !aocmd_cint_parse_hex(argv[6],&count) || count<1 || count>8 ) { Serial.printf("ERROR: 'read' expects <count> 1..8, not '%s'\n",argv[6]); return; }
  } else {
    Serial.printf("ERROR: 'read' has too many args\n"); return;
  }
  // Now read
  #define RBUFSIZE 8
  uint8_t buf[RBUFSIZE];
  aoresult_t result= aoosp_exec_i2cread8(addr, daddr7, raddr, buf, count);
  // Feedback
  if( result!=aoresult_ok ) { Serial.printf("ERROR: read(%03X) failed (%s)\n", addr, aoresult_to_str(result) ); return; }
  if( argv[0][0]!='@' ) Serial.printf("said(%03X).i2c.dev(%02X).reg(%02X) ",addr,daddr7,raddr );
  Serial.printf("%s\n", aoosp_prt_bytes(buf,count) );
}


// Parse 'said i2c <addr> ( scan | freq [<freq>] | write <daddr7> <raddr> <data>... | read <daddr7> <raddr> <count> )'
static void aocmd_said_i2c( int argc, char * argv[] ) {
  if( argc<3 ) { Serial.printf("ERROR: i2c requires <addr>\n"); return; }

  // get <addr>
  uint16_t addr;
  if( !aocmd_cint_parse_hex(argv[2],&addr) || !AOOSP_ADDR_ISOK(addr) || OAOSP_ADDR_ISMULTICAST(addr) ) {
    Serial.printf("ERROR: illegal <addr> '%s'\n",argv[2]);
    return;
  }

  if( AOOSP_ADDR_ISUNICAST(addr) ) { // 'said i2c 000 scan' allows broadcast, skip next check
    aoresult_t result= aoosp_exec_i2cpower(addr);
    if( result==aoresult_sys_id ) { Serial.printf("ERROR: not a SAID at %03x\n", addr ); return; }
    if( result==aoresult_dev_noi2cbridge ) { Serial.printf("ERROR: SAID at %03x has no I2C (OTP bit not set)\n", addr ); return; }
    if( result!=aoresult_ok ) { Serial.printf("ERROR: i2cpower(%03X) failed (%s) - forgot 'osp resetinit'?\n", addr, aoresult_to_str(result) ); return; }
  }

  if( argc<4 ) { Serial.printf("ERROR: 'i2c' expects 'scan', 'freq', 'write', or 'read'\n"); return; }

  if( aocmd_cint_isprefix("scan",argv[3]) ) {
    if( argc!=4 ) { Serial.printf("ERROR: 'scan' has unknown argument ('%s')\n", argv[4]); return; }
    if( AOOSP_ADDR_ISUNICAST(addr) ) {
      aocmd_said_i2c_scan_uni(addr,argv[0][0]!='@');
    } else {
      aocmd_said_i2c_scan_broad(argv[0][0]!='@');
    }
  } else if( aocmd_cint_isprefix("freq",argv[3]) ) {
    aocmd_said_i2c_freq(argc,argv,addr);
  } else if( aocmd_cint_isprefix("write",argv[3]) ) {
    aocmd_said_i2c_write(argc,argv,addr);
  } else if( aocmd_cint_isprefix("read",argv[3]) ) {
    aocmd_said_i2c_read(argc,argv,addr);
  } else {
    Serial.printf("ERROR: 'i2c' has unknown argument ('%s')\n", argv[3]); return;
  }
}


// Parse 'said otp <addr> [ <otpaddr> [ <data> ] ]'
static void aocmd_said_otp( int argc, char * argv[] ) {
  aoresult_t result;

  if( argc<3 ) { Serial.printf("ERROR: 'otp' expects <addr> of SAID\n"); return; }

  // get <addr>
  uint16_t addr;
  if( !aocmd_cint_parse_hex(argv[2],&addr) || !AOOSP_ADDR_ISUNICAST(addr) ) {
    Serial.printf("ERROR: 'otp' expects <addr> %03X..%03X, not '%s'\n",AOOSP_ADDR_UNICASTMIN,AOOSP_ADDR_UNICASTMAX,argv[2]);
    return;
  }

  // Check if it is a SAID
  uint32_t id;
  result = aoosp_send_identify(addr, &id );
  if( result!=aoresult_ok ) { Serial.printf("ERROR: identify(%03X) failed (%s) - forgot 'osp resetinit'?\n", addr, aoresult_to_str(result) ); return; }
  if( ! AOOSP_IDENTIFY_IS_SAID(id) ) { Serial.printf("ERROR: node %03X is not a SAID (id %08lX)\n", addr,id ); return; }

  // Action: Dump
  if( argc==3 ) {
    aoosp_exec_otpdump(addr, AOOSP_OTPDUMP_CUSTOMER_HEX | AOOSP_OTPDUMP_CUSTOMER_FIELDS );
    return;
  }

  // get <otpaddr>
  uint16_t otpaddr;
  if( !aocmd_cint_parse_hex(argv[3],&otpaddr) || otpaddr<AOOSP_OTPADDR_CUSTOMER_MIN || otpaddr>AOOSP_OTPADDR_CUSTOMER_MAX ) {
    Serial.printf("ERROR: 'otp' expects <otpaddr> %02X..%02X, not '%s'\n",AOOSP_OTPADDR_CUSTOMER_MIN,AOOSP_OTPADDR_CUSTOMER_MAX,argv[3]);
    return;
  }

  // Action: Read
  if( argc==4 ) {
    uint8_t data;
    result = aoosp_send_readotp(addr, otpaddr, &data, 1);
    Serial.printf("SAID[%03X].OTP[%02X] -> %02X (%s)\n", addr, otpaddr, data, aoresult_to_str(result) );
    return;
  }

  // get <data>
  uint16_t data;
  if( !aocmd_cint_parse_hex(argv[4],&data) || data>0xFF ) {
    Serial.printf("ERROR: illegal <data> '%s' (0x00..0xFF)\n",argv[2]);
    return;
  }

  // Action: write
  if( argc>5 ) { Serial.printf("ERROR: 'otp' has too many args\n"); return; }
  result = aoosp_exec_setotp(addr, otpaddr, data, 0x00);
  if( argv[0][0]!='@' ) Serial.printf("SAID[%03X].OTP[%02X] <- %02X (%s)\n", addr, otpaddr, data, aoresult_to_str(result) );
}



// Print the SAID password as it is registered
static void aocmd_said_password_show() {
  uint64_t pw = aoosp_said_testpw_get();
  Serial.printf("stored password: %012llX\n", pw  );
}


// Parse 'said password [ <pw> ]'
static void aocmd_said_password( int argc, char * argv[] ) {
  if( argc==2 ) { 
    aocmd_said_password_show();
  } else if( argc==3 ) { 
    const char * s= argv[2];
    int len = strlen(s);
    if( len>12 ) { Serial.printf("ERROR: password too long\n"); return; }
    uint64_t pw = 0;
    for( int i=0; i<len; i++ ) {
      char ch = s[i];
      if( '0'<=ch && ch<='9' ) pw= pw*16 + (ch-'0');
      else if( 'a'<=ch && ch<='f' ) pw= pw*16 + (ch-'a'+10);
      else if( 'A'<=ch && ch<='F' ) pw= pw*16 + (ch-'A'+10);
      else { Serial.printf("ERROR: password expects hex chars, not '%c'\n", ch); return; }
    }
    aoosp_said_testpw_set(pw);
    if( argv[0][0]!='@' ) aocmd_said_password_show();
  } else {
    Serial.printf("ERROR: 'password' has too many args\n"); 
  }
}


// The handler for the "said" command
static void aocmd_said_main( int argc, char * argv[] ) {
  if( aocmd_cint_isprefix("password",argv[1]) ) {
    aocmd_said_password(argc, argv);
    return;
  }
  
  if( aoosp_exec_resetinit_last()==0 ) Serial.printf("WARNING: 'osp resetinit' must be run first\n");
  
  if( argc==1 ) {
    Serial.printf("ERROR: 'said' expects argument\n"); return;
  } else if( aocmd_cint_isprefix("i2c",argv[1]) ) {
    aocmd_said_i2c(argc, argv);
  } else if( aocmd_cint_isprefix("otp",argv[1]) ) {
    aocmd_said_otp(argc, argv);
  } else {
    Serial.printf("ERROR: 'said' has unknown argument ('%s')\n", argv[1]); return;
  }
}


// The long help text for the "said" command.
static const char aocmd_said_longhelp[] =
  "SYNTAX: said i2c <addr> ( scan | freq [<freq>] | <rw> )\n"
  "- checks <addr> is a SAID with I2C enabled (OTP), if so powers bus, then\n"
  "- 'scan' scans for I2C devices on bus (<addr> 000 loops over entire chain)\n"
  "- 'freq' gets or sets I2C bus frequency (in Hz)\n"
  "- <rw> can be 'write' <daddr7> <raddr> <data>...\n"
  "- this writes the <data> bytes to register <raddr> of i2c device <daddr7>\n"
  "- <rw> can be 'read <daddr7> <raddr> [<count>]'\n"
  "- this reads <count> bytes from register <raddr> of i2c device <daddr7>\n"
  "SYNTAX: said otp <addr> [ <otpaddr> [ <data> ] ]\n"
  "- read/writes OTP memory (customer area) of the SAID at address <addr>\n"
  "- without optional arguments dumps OTP memory\n"
  "- with <otpaddr> reads OTP location <otpaddr>\n"
  "- with <data> writes <data> to OTP location <otpaddr>\n"
  "SYNTAX: said password [ <pw> ]\n"
  "- without optional argument shows the SAID test password in the firmware\n"
  "- with <pw> sets it (FFFFFFFFFFFF triggers warning when PW is needed)\n"
  "NOTES:\n"
  "- supports @-prefix to suppress output\n"
  "- commands assume chain is initialized (e.g. 'osp resetinit')\n"
  "- <addr> is a node address in hex (001..3EA, 000 for broadcast, 3Fx for group)\n"
  "- <otpdata> is a 8-bit OTP address in hex (00..FF)\n"
  "- <daddr7> is a 7-bit I2C device address in hex (00..7F)\n"
  "- <raddr> is a 8-bit I2C register address in hex (00..FF)\n"
  "- <data> is a 8-bit argument in hex (00..FF)\n"
;


/*!
    @brief  Registers the built-in "said" command with the command interpreter.
    @return Number of remaining registration slots (or -1 if registration failed).
    @note   The aocmd_init calls this function, so normal client code does not need to call it.
    @note   If client code overrides the default registration by implementing
            its own aocmd_register() then this function could be called from there.
*/
int aocmd_said_register() {
  return aocmd_cint_register(aocmd_said_main, "said", "sends and receives SAID specific telegrams", aocmd_said_longhelp);
}
