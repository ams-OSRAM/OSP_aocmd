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



static int aocmd_said_i2c_scan(uint16_t addr) {
  // Power the I2C bus
  aoresult_t result= aoosp_exec_i2cpower(addr);
  if(result!=aoresult_ok) { Serial.printf("ERROR: aoosp_exec_i2cpower(%03X) failed (%s)\n", addr, aoresult_to_str(result) ); return 0; }
  
  // Scan all i2c devices
  Serial.printf("SAID %03X has I2C (now powered)\n",addr);
  int count= 0;
  for( uint8_t daddr7=0; daddr7<0x80; daddr7++ ) {
    if( daddr7 % 16 == 0) Serial.printf("  %02x: ",daddr7);
    // Try to read at address 0 of device daddr7
    uint8_t buf[8];
    result = aoosp_exec_i2cread8(addr, daddr7, 0x00, buf, 1);
    int i2cfail=  result==aoresult_dev_i2cnack || result==aoresult_dev_i2ctimeout;
    if( result!=aoresult_ok && !i2cfail ) { Serial.printf("ERROR: aoosp_exec_i2cread8(%03X) failed (%s)\n", addr, aoresult_to_str(result) ); return 0; }
    if( i2cfail ) Serial.printf(" %02x ",daddr7); else Serial.printf("[%02x]",daddr7); // [] brackets indicate presence
    if( !i2cfail ) count++;
    if( daddr7 % 16 == 15) Serial.printf("\n");
  }
  Serial.printf("SAID %03X has %d I2C devices\n",addr, count);
  return count;
}


// Parse 'said i2c <addr> scan'
// todo: add "said i2c ( search | pow <addr> | cfg <addr> <freq> | scan <addr> | write <addr> <daddr> <raddr> <data>... | read <addr> <daddr> <raddr> <num> )
static void aocmd_said_i2c( int argc, char * argv[] ) {
  if( argc<3 ) { Serial.printf("ERROR: i2c requires <addr>\n"); return; }
  
  // get <addr>
  uint16_t addr;
  if( !aocmd_cint_parse_hex(argv[2],&addr) || !AOOSP_ADDR_ISOK(addr) || OAOSP_ADDR_ISMULTICAST(addr) ) {
    Serial.printf("ERROR: illegal <addr> '%s'\n",argv[2]);
    return;
  }

  if( argc==3 ) {
    // assume 'scan' for now
  } else if( argc==4 ) {
    if( ! aocmd_cint_isprefix("scan",argv[3]) ) { Serial.printf("ERROR: unknown i2c command\n"); return; }
  } else { 
    Serial.printf("ERROR: too many arguments\n"); return; 
  }

  aoresult_t result;
  if( AOOSP_ADDR_ISUNICAST(addr) ) {
    // Is this (addr) a SAID?
    uint32_t id;
    result = aoosp_send_identify(addr, &id );
    if( result!=aoresult_ok ) { Serial.printf("ERROR: identify(%03X) failed (%s) - forgot 'osp resetinit'?\n", addr, aoresult_to_str(result) ); return; }
    if( ! AOOSP_IDENTIFY_IS_SAID(id) ) { Serial.printf("ERROR: node %03X is not a SAID (id %08lX)\n", addr,id ); return; }
    // Is this SAID having I2C bridge enabled?
    int enable;
    result = aoosp_exec_i2cenable_get(addr, &enable);
    if(result!=aoresult_ok) { Serial.printf("ERROR: aoosp_exec_i2cenable_get(%03X) failed (%s)\n", addr, aoresult_to_str(result) ); return; }
    if(!enable ) { Serial.printf("ERROR: SAID at %03X does not have i2c bridge (OTP bit not set)\n", addr ); return; }
    aocmd_said_i2c_scan(addr);
  } else {
    // Broadcast is interpreted as looping over chain
    // resetinit forgotten?
    uint32_t id;
    result = aoosp_send_identify(0x001, &id );
    if( result!=aoresult_ok ) { Serial.printf("ERROR: identify(001) failed (%s) - forgot 'osp resetinit'?\n", aoresult_to_str(result) ); return; }
    // Scan all OSP nodes
    int i2ccount=0;
    int saidcount=0;
    for( int addr=1; addr<=AOOSP_ADDR_MAX; addr++ ) {
      // Is this (addr) a SAID?
      uint32_t id;
      result = aoosp_send_identify(addr, &id );
      if( result==aoresult_spi_length ) break; // end of chain
      if( result!=aoresult_ok ) { Serial.printf("ERROR: aoosp_send_identify(%03X) failed (%s)\n", addr, aoresult_to_str(result) ); return; }
      if( ! AOOSP_IDENTIFY_IS_SAID(id) ) continue;
      
      // Is this SAID having I2C bridge enabled?
      int enable;
      result = aoosp_exec_i2cenable_get(addr, &enable);
      if(result!=aoresult_ok) { Serial.printf("ERROR: aoosp_exec_i2cenable_get(%03X) failed (%s)\n", addr, aoresult_to_str(result) ); return; }
      if( ! enable ) continue;
      
      // Scan SAID at addr
      i2ccount+= aocmd_said_i2c_scan(addr);
      saidcount++;
      Serial.printf("\n");
    }
    Serial.printf("%d SAIDs have %d I2C devices\n", saidcount, i2ccount);
  }
    
}


// Parse 'said otp <addr> [ <otpaddr> [ <data> ] ]'
static void aocmd_said_otp( int argc, char * argv[] ) {
  aoresult_t result;
  
  if( argc<3 ) { Serial.printf("ERROR: otp requires <addr> of SAID\n"); return; }
  
  // get <addr>
  uint16_t addr;
  if( !aocmd_cint_parse_hex(argv[2],&addr) || !AOOSP_ADDR_ISUNICAST(addr) ) {
    Serial.printf("ERROR: illegal <addr> '%s'\n",argv[2]);
    return;
  }

  // Check if it is a SAID
  uint32_t id;
  result = aoosp_send_identify(addr, &id );
  if( result!=aoresult_ok ) { Serial.printf("ERROR: identify(%03X) failed (%s) - forgot 'osp resetinit'?\n", addr, aoresult_to_str(result) ); return; }
  if( ! AOOSP_IDENTIFY_IS_SAID(id) ) { Serial.printf("ERROR: node %03X is not a SAID (id %08lX)\n", addr,id ); return; }

  // Dump  
  if( argc==3 ) {
    aoosp_exec_otpdump(addr, AOOSP_OTPDUMP_CUSTOMER_HEX | AOOSP_OTPDUMP_CUSTOMER_FIELDS );
    return;
  }

  // get <otpaddr>
  uint16_t otpaddr;
  if( !aocmd_cint_parse_hex(argv[3],&otpaddr) || otpaddr>0x20 ) {
    Serial.printf("ERROR: illegal <otpaddr> '%s' (0x00..0x20)\n",argv[2]);
    return;
  }
  
  // Read
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

  // write
  if( argc>5 ) { Serial.printf("ERROR: otp has too many args\n"); return; }
  result = aoosp_exec_setotp(addr, otpaddr, data, 0x00); 
  if( argv[0][0]!='@' ) Serial.printf("SAID[%03X].OTP[%02X] <- %02X (%s)\n", addr, otpaddr, data, aoresult_to_str(result) );
}


// The handler for the "said" command
static void aocmd_said_main( int argc, char * argv[] ) {
  if( argc==1 ) {
    Serial.printf("ERROR: missing argument\n"); return;
  } else if( aocmd_cint_isprefix("i2c",argv[1]) ) {
    aocmd_said_i2c(argc, argv);
  } else if( aocmd_cint_isprefix("otp",argv[1]) ) {
    aocmd_said_otp(argc, argv);
  } else {
    Serial.printf("ERROR: unknown argument\n"); return;
  }
}


// The long help text for the "said" command.
static const char aocmd_said_longhelp[] =
  "SYNTAX: said i2c <addr> [scan]\n"
  "- checks if <addr> is a SAID with i2c enabled in OTP, if so powers and scans bus\n"
  "- if <addr> is 000, loops over entire chain\n"
  "- at this moment 'scan' is only available sub command, so it may be left out\n"
  "SYNTAX: said otp <addr> [ <otpaddr> [ <data> ] ]\n"
  "- read/writes OTP memory of the SAID at address <addr>\n"
  "- without optional arguments dumps entire OTP memory\n"
  "- with <otpaddr> reads OTP location <otpaddr>\n"
  "- with <data> writes <data> to OTP location\n"
  "NOTES:\n"
  "- commands assume chain is initialized (e.g. 'osp resetinit')\n"
  "- <addr> is a node address in hex (1..3EA, 0 for broadcast, 3Fx for group)\n"
  "- <otpdata> is a (one-byte) address in hex 00..FF\n"
  "- <data> is a (one-byte) argument in hex 00..FF\n"
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
