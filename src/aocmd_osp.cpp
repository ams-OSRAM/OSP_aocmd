// aocmd_osp.cpp - command handler for the "osp" command - to send and receive OSP telegrams
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
#include <string.h>         // strchrnul
#include <aospi.h>          // aospi_dirmux_set_bidir, aospi_tx, ...
#include <aoosp.h>          // aoosp_crc()
#include <aocmd_cint.h>     // aocmd_cint_register, aocmd_cint_isprefix, ...
#include <aocmd_osp.h>      // own


#define BITS_MASK(n)         ((1<<(n))-1)                            // series of n bits: BITS_MASK(3)=0b111 (max n=31)
#define BITS_SLICE(v,lo,hi)  ( ((v)>>(lo)) & BITS_MASK((hi)-(lo)) )  // takes bits [lo..hi) from v: BITS_SLICE(0b11101011,2,6)=0b1010
#define PSI(payloadsize)     ( (payloadsize)<8 ? (payloadsize) : 7 ) // convert "human" payload size, to "telegram" payload size


// Telegram id's, or tid's for short, are published by OSP and fixed.
// For example tid 0x05 is reserved for telegram GOACTIVE.
// However, some tid's are reused. For example 0x4E is for READPWM and for READPWMCHN.
// Although these share the same intention, the (amount of) arguments varies.
// The table below `aocmd_osp_variant[]`, distinguishes between them.
// The two variants have a different row into the table.
//
// To summarize, we distinguish
//  - telegram id (tid)
//  - telegram variant index (vix)
//
// The tid's are (by definition of OSP) always in the range of 00..0x7F.
// The vix's are much more unbounded, every node type could introduce their own variants.


// Struct with info on one telegram variant
typedef struct aocmd_osp_variant {
  int          tid;         // telegram id (0x00..0x7F)
  const char * swname;      // name in the software api
  int          serial;      // command uses serial-cast
  int          minsize;     // payload size (minimal) in bytes
  int          maxsize;     // payload size (maximum) in bytes
  int          respsize;    // response payload size (0 means no response) in bytes
  const char * teleargs;    // description of the telegram args
  const char * respargs;    // description of the response args
  const char * description; // one sentence description
} aocmd_osp_variant_t;


// Array with info on all telegram variants
static const aocmd_osp_variant_t aocmd_osp_variant[] = {
  #define ITEM(tid,swname,serial,minsize,maxsize,respsize,teleargs,respargs,description) \
    { tid, swname, serial, minsize, maxsize, respsize, teleargs, respargs, description },
  #include <aocmd_osp.i>
};
#define AOCMD_OSP_VARIANT_COUNT                    ( sizeof(aocmd_osp_variant)/sizeof(aocmd_osp_variant_t) )

#define AOCMD_OSP_VARIANT_HAS_UNICAST(var)         ( (var)->serial==0 )
#define AOCMD_OSP_VARIANT_HAS_SERIALCAST(var)      ( (var)->serial!=0 )
#define AOCMD_OSP_VARIANT_HAS_BROADMULTICAST(var)  ( (var)->respsize==0 && (var)->serial==0 )

#define AOCMD_OSP_VARIANT_HAS_RESPONSE(var)        ( (var)->respsize>0 )
#define AOCMD_OSP_VARIANT_HAS_SR_VARIANT(var)      ( (var)->respsize==0 && (var)->serial==0 && (var)->tid!=0 )
#define AOCMD_OSP_VARIANT_IS_SR_VARIANT(var)       ( (var)->tid & (1<<5) )

#define AOCMD_OSP_VARIANT_HAS_INFO(var)            ( (var)->swname!=0 )


// Struct mapping one tid to num vix's
typedef struct aocmd_osp_tidmap {
  int          num;         // Number of variants
  int          vix;         // First variant for the tid (entry into aocmd_osp_variant[]), undef if num==0
} aocmd_osp_tidmap_t;


// Lookup table from tid's to vix's (note tid are 7 bit in OSP)
static aocmd_osp_tidmap_t aocmd_osp_tidmap[0x80];


/*!
    @brief  Initializes the telegram parser.
    @note   Also performs sanity check on telegram variant info,
            aborts upon failure (with assert message).
*/
void aocmd_osp_init() {
  int prevtid=0;
  for( int vix=0; vix<AOCMD_OSP_VARIANT_COUNT; vix++ ) {
    const aocmd_osp_variant_t * var = & aocmd_osp_variant[vix];
    AORESULT_ASSERT( 0<=var->tid && var->tid<0x80 );
    AORESULT_ASSERT( var->tid==prevtid || var->tid==prevtid+1 ); // all tid's must occur, increasing (doubles allowed)
    if( AOCMD_OSP_VARIANT_HAS_INFO(var) ) {
      AORESULT_ASSERT( var->swname!=0 );
      AORESULT_ASSERT( var->serial==0 || var->serial==1 );
      AORESULT_ASSERT( 0<=var->minsize && var->minsize<=var->maxsize && var->maxsize<=8 );
      AORESULT_ASSERT( 0<=var->respsize && var->respsize<=8 );
      AORESULT_ASSERT( var->maxsize>0 || var->teleargs==0 );
      AORESULT_ASSERT( var->respsize>0 || var->respargs==0 );
      AORESULT_ASSERT( var->description!=0 );
    } else {
      AORESULT_ASSERT( var->swname==0 );
      AORESULT_ASSERT( var->serial==0 );
      AORESULT_ASSERT( var->minsize==0 );
      AORESULT_ASSERT( var->maxsize==0 );
      AORESULT_ASSERT( var->respsize==0 );
      AORESULT_ASSERT( var->teleargs==0 );
      AORESULT_ASSERT( var->respargs==0 );
      AORESULT_ASSERT( var->description==0 );
    }
    prevtid= var->tid;
  }
  AORESULT_ASSERT( prevtid==0x7F ); // all tid's must occur

  // Populate the tidmap table
  int vix=0;
  for( int tid=0; tid<0x80; tid++ ) {
    if( vix==AOCMD_OSP_VARIANT_COUNT || aocmd_osp_variant[vix].tid>tid ) {
      // No registered variant for tid
      aocmd_osp_tidmap[tid].num= 0;
      aocmd_osp_tidmap[tid].vix= -1;
    } else {
      AORESULT_ASSERT( aocmd_osp_variant[vix].tid==tid );
      aocmd_osp_tidmap[tid].num= 0;
      aocmd_osp_tidmap[tid].vix= vix;
      while( aocmd_osp_variant[vix].tid==tid ) {
        aocmd_osp_tidmap[tid].num++;
        vix++;
      }
    }
  }
  AORESULT_ASSERT( vix==AOCMD_OSP_VARIANT_COUNT );
  AORESULT_ASSERT( aocmd_osp_tidmap[0x7F].vix!=0 ); // init has run 
}


// Prints `variant` in human friendly format to Serial
static void aocmd_osp_variant_print( const aocmd_osp_variant_t * variant ) {
  Serial.printf("TELEGRAM %02X: ", variant->tid );
  if( ! AOCMD_OSP_VARIANT_HAS_INFO(variant) ) {
    Serial.printf("no info on telegram\n\n"); return;
  }
  Serial.printf("%s\n", variant->swname );

  #define LEN 65
  #define INDENT1 "DESCRIPTION:"
  #define INDENT2 "            "
  const char * indent = INDENT1;
  const char * str = variant->description;
  int len = strlen(str);
  while( len>0 ) {
    if( len<=LEN ) {
      Serial.printf("%s %s\n",indent,str);
      str+=len; len-=len;
    } else {
      const char * spc1 = str;
      const char * spc2 = strchrnul(spc1,' ');
      while( spc2-str<LEN ) {
        spc1=spc2+1; spc2=strchrnul(spc1,' ');
      }
      int num= spc1-str;
      if( num==0 ) num= LEN; // cut anyhow if no space found
      Serial.printf("%s %.*s\n",indent,num,str);
      str+=num; len-=num;
    }
    indent = INDENT2;
  }

  Serial.printf("CASTING    : ");
  if( AOCMD_OSP_VARIANT_HAS_UNICAST(variant)        ) Serial.printf("uni ");
  if( AOCMD_OSP_VARIANT_HAS_SERIALCAST(variant)     ) Serial.printf("serial ");
  if( AOCMD_OSP_VARIANT_HAS_BROADMULTICAST(variant) ) Serial.printf("multi ");
  if( AOCMD_OSP_VARIANT_HAS_BROADMULTICAST(variant) ) Serial.printf("broad ");
  Serial.printf("\n");

  Serial.printf("PAYLOAD    : %d", variant->minsize);
  if( variant->minsize != variant->maxsize ) Serial.printf("-%d",variant->maxsize);
  if( variant->maxsize>0 ) Serial.printf(" (%s)", variant->teleargs);
  if( AOCMD_OSP_VARIANT_HAS_RESPONSE(variant) ) Serial.printf("; response %d (%s)",variant->respsize,variant->respargs ); else Serial.printf("; no response");
  Serial.printf("\n");
  Serial.printf("STATUS REQ : ");
  const aocmd_osp_variant_t * altvar = & aocmd_osp_variant[aocmd_osp_tidmap[ variant->tid ^ (1<<5) ].vix];
  if( AOCMD_OSP_VARIANT_IS_SR_VARIANT(variant) ) {
    Serial.printf("yes");
    Serial.printf(" (tele %02X/%s has none)", altvar->tid, altvar->swname );
  } else {
    Serial.printf("no");
    if( AOCMD_OSP_VARIANT_HAS_SR_VARIANT(variant) && AOCMD_OSP_VARIANT_HAS_INFO(altvar) ) {
      Serial.printf(" (tele %02X/%s has sr)", altvar->tid, altvar->swname );
    } else {
      Serial.printf(" (no sr possible)" );
    }
  }
  Serial.printf("\n");

  int duplicate_found = 0;
  int tid = variant->tid;
  for( int i=0; i<AOCMD_OSP_VARIANT_COUNT; i++ ) {
    if( aocmd_osp_variant[i].tid==tid && &aocmd_osp_variant[i]!=variant ) {
      if( ! duplicate_found ) Serial.printf("DUPLICATE  : ");
      Serial.printf("%02X/%s ", aocmd_osp_variant[i].tid, aocmd_osp_variant[i].swname );
      duplicate_found= 1;
    }
  }
  if( duplicate_found ) Serial.printf("\n");

  Serial.printf("\n"); // final white line
}


// Finds a variant in the table, using a human entered `key` (from Serial).
// The key could be a hex tid, or a (part of a) telegram name.
// Multiple variants could match the `key`. The function will populate the caller
// allocated array `variants` of size `size`. Returns number of variants found.
static int aocmd_osp_variant_find( const char * key, int * variants, int size) {
  int found = 0;

  // Is `key` a hex number for a tid?
  uint16_t tid;
  bool ok= aocmd_cint_parse_hex(key,&tid) ;
  if( strlen(key)==2 && isdigit(key[0]) && ok ) {
    for( int i=0; i<aocmd_osp_tidmap[tid].num; i++ ) {
      if( found<size ) variants[found++]= aocmd_osp_tidmap[tid].vix+i;
    }
    return found; // Only return numeric matches
  }

  // Is `key` and exact match of some telegram name?
  for( int vix=0; vix<AOCMD_OSP_VARIANT_COUNT; vix++ ) {
    if( aocmd_osp_variant[vix].swname!=0 && strcmp( aocmd_osp_variant[vix].swname, key)==0 ) {
      if( found<size ) variants[found++]= vix;
      return found; // Only return the one exact match
    }
  }

  // Is `key` and infix match of some telegram name?
  for( int vix=0; vix<AOCMD_OSP_VARIANT_COUNT; vix++ ) {
    if( aocmd_osp_variant[vix].swname!=0 && strstr(aocmd_osp_variant[vix].swname,key)!=0 ) {
      if( found<size ) variants[found++]= vix;
    }
  }

  return found;
}


// === handler for "osp" ===================================================


// Local status
static int oacmd_osp_validate = 1;


// Shows status
static void aocmd_osp_dirmux_show() {
  Serial.printf("dirmux: %s\n", aospi_dirmux_is_loop() ? "loop" : "bidir" );
}


// Show validation status
static void aocmd_osp_validate_show() {
  Serial.printf("validate: %s\n", oacmd_osp_validate ? "enabled" : "disabled" );
}


// Show tx/rx counter status
static void aocmd_osp_count_show() {
  Serial.printf("count: tx %d rx %d\n", aospi_txcount_get(), aospi_rxcount_get() );
}


// shows log status
static void aocmd_osp_log_show() {
  Serial.printf("log: " );
  if( aoosp_loglevel_get()==aoosp_loglevel_none ) Serial.printf("none");
  if( aoosp_loglevel_get()==aoosp_loglevel_args ) Serial.printf("args");
  if( aoosp_loglevel_get()==aoosp_loglevel_tele ) Serial.printf("tele");
  Serial.printf("\n");
}


// Show status of the output enable of the outgoing level shifter
static void aocmd_osp_hwtestout_show() {
  Serial.printf("test out: %s\n", aospi_outoena_get() ? "enabled" : "disabled" );
}


// Show status of the output enable of the incoming level shifter
static void aocmd_osp_hwtestin_show() {
  Serial.printf("test in : %s\n", aospi_inoena_get() ? "enabled" : "disabled" );
}


// Shows list of telegrams with info 
static void aocmd_osp_info_show() {
  int printed=0;
  for( int vix=0; vix<AOCMD_OSP_VARIANT_COUNT; vix++ ) {
    const aocmd_osp_variant_t * var = & aocmd_osp_variant[vix];
    if( AOCMD_OSP_VARIANT_HAS_INFO(var) ) {
      Serial.printf("%02X/%-16s", var->tid, var->swname );
      printed++;
      if( printed%4==0 ) Serial.printf("\n"); else  Serial.printf(" ");
    }
  }
  if( printed%4!=0 ) Serial.printf("\n");
}


// Parse 'osp send <addr> <tele> <arg>...', validate, send, optionally receive
static void aocmd_osp_send( int argc, char * argv[] ) {
  if( argc<4   ) { Serial.printf("ERROR: expected <addr> <tele> <args>...\n"); return; }
  if( argc>4+8 ) { Serial.printf("ERROR: too many <args>\n"); return; }
  int payloadsize = argc-4;

  // get <addr>
  uint16_t addr;
  if( !aocmd_cint_parse_hex(argv[2],&addr) || !AOOSP_ADDR_ISOK(addr) ) {
    Serial.printf("ERROR: illegal <addr> '%s'\n",argv[2]);
    return;
  }

  // get <tele>
  #define SEND_FINDMAX 8
  int variants[SEND_FINDMAX];
  int found = aocmd_osp_variant_find( argv[3], variants, SEND_FINDMAX);
  const aocmd_osp_variant_t * var= 0;
  if( found==0 ) {
    Serial.printf("ERROR: no <tele> '%s' found\n", argv[3]);
    return;
  } else if( found==1 ) {
    var = &aocmd_osp_variant[variants[0]];
  } else {
    // The only way to have a variant without info is if the user entered a hex number.
    // But for variants with no details, there are never two with the same <hex>.
    AORESULT_ASSERT( AOCMD_OSP_VARIANT_HAS_INFO( &aocmd_osp_variant[variants[0]]) );
    if( aocmd_osp_variant[variants[0]].tid==aocmd_osp_variant[variants[1]].tid ) {
      // [case 1] <tele> has <hex> form and maps to multiple variants (e.g. "4F" for "setpwm" and "setpwmchn"),
      // Find variant using matching payload size
      for( int i=0; i<found; i++ ) {
        if( aocmd_osp_variant[variants[i]].minsize<=payloadsize && payloadsize<=aocmd_osp_variant[variants[i]].maxsize )
          var= &aocmd_osp_variant[variants[i]];
      }
      if( var==0 ) var= &aocmd_osp_variant[variants[0]]; // None fits; just pick first
    } else {
      // [case 2] or <tele> has form <str> and multiple names match (e.g. "setup" for "readsetup" and "setsetup"
      var = &aocmd_osp_variant[variants[0]]; // Just pick first
    }
  }

  // get <arg>... already in tx[]
  uint8_t tx[AOSPI_TELE_MAXSIZE];
  for( int tix=3, aix=4; aix<argc; aix++, tix++ ) { // tix index in tx[], aix index in argv[]
    uint16_t arg;
    bool ok= aocmd_cint_parse_hex(argv[aix],&arg) ;
    if( !ok || arg>0xFF ) { Serial.printf("ERROR: illegal <arg> '%s'\n",argv[aix]); return; }
    tx[tix] = arg;
  }

  // Constructing rest of telegram
  tx[0] = 0xA0 | BITS_SLICE(addr,6,10);
  tx[1] = BITS_SLICE(addr,0,6)<<2 | BITS_SLICE(PSI(payloadsize),1,3);
  tx[2] = BITS_SLICE(PSI(payloadsize),0,1)<<7 | var->tid;
  tx[3+payloadsize] = aoosp_crc(tx,3+payloadsize);

  // Validation
  if( oacmd_osp_validate ) {
    if( AOCMD_OSP_VARIANT_HAS_INFO(var) ) {
      // Validate payload size
      if( payloadsize < var->minsize  ||  var->maxsize < payloadsize ) {
        Serial.printf("validate: %02X/%s does not have %d bytes as payload, but",var->tid,var->swname,payloadsize);
        for( int i=0; i<found; i++ ) Serial.printf(" %d..%d", aocmd_osp_variant[variants[i]].minsize, aocmd_osp_variant[variants[i]].maxsize );
        Serial.printf("\n");
      }
      if( payloadsize==5 || payloadsize==7 ) Serial.printf("validate: payload can not have size 5 or 7\n");
      // Validate addressing
      if( AOOSP_ADDR_ISBROADCAST(addr) && !AOCMD_OSP_VARIANT_HAS_BROADMULTICAST(var) ) Serial.printf("validate: %02X/%s does not support broadcast\n",var->tid,var->swname);
      if( OAOSP_ADDR_ISMULTICAST(addr) && !AOCMD_OSP_VARIANT_HAS_BROADMULTICAST(var) ) Serial.printf("validate: %02X/%s does not support multicast\n",var->tid,var->swname);
    } else {
      // Validate: no info available
      Serial.printf("validate: no info %02X/??? to validate against\n",var->tid);
    }
  }
  if( argv[0][0]!='@' ) Serial.printf("tx %s\n", aoosp_prt_bytes(tx,4+payloadsize) );

  // Execute
  uint8_t rx[AOSPI_TELE_MAXSIZE];
  memset(rx,0xA5,AOSPI_TELE_MAXSIZE);
  aoresult_t result;
  if( AOCMD_OSP_VARIANT_HAS_INFO(var) ) {
    if( AOCMD_OSP_VARIANT_HAS_RESPONSE(var) ) {
      result = aospi_txrx(tx, payloadsize+4, rx, var->respsize+4);
      Serial.printf("rx %s",aoosp_prt_bytes(rx,var->respsize+4));
    } else {
      result = aospi_tx(tx, payloadsize+4);
      Serial.printf("rx none");
    }
  } else {
    int actsize;
    result = aospi_txrx(tx, payloadsize+4, rx, AOSPI_TELE_MAXSIZE, &actsize );
    Serial.printf("rx %s", aoosp_prt_bytes(rx,actsize));
  }
  Serial.printf(" %s\n",aoresult_to_str(result));
}


// Parse 'osp (tx|trx) <data>... [crc]', validate, send, optionally receive
static void aocmd_osp_trx( int argc, char * argv[] ) {
  uint8_t tx[AOSPI_TELE_MAXSIZE];

  for( int tix=0, aix=2; aix<argc; aix++, tix++ ) { // tix index in tx[], aix index in argv[]
    if( aix==argc-1 && aocmd_cint_isprefix("crc",argv[aix]) ) {
      tx[tix] = aoosp_crc(tx,tix);
    } else {
      uint16_t arg;
      bool ok= aocmd_cint_parse_hex(argv[aix],&arg) ;
      if( !ok || arg>0xFF ) { Serial.printf("ERROR: illegal <data> '%s'\n",argv[aix]); return; }
      tx[tix] = arg;
    }
  }
  int telesize = argc-2;
  int payloadsize = telesize-4;

  // Validation
  if( oacmd_osp_validate ) {
    if( payloadsize<0 ) {
      Serial.printf("validate: minimal telegram length is 4 bytes (other validation skipped)\n");
    } else {
      // dissect bytes
      int preamble = BITS_SLICE(tx[0],4,8);
      int addr= (BITS_SLICE(tx[0],0,4)<<6) + BITS_SLICE(tx[1],2,8);
      int psi= (BITS_SLICE(tx[1],0,2)<<1) + BITS_SLICE(tx[2],7,8);
      int tid= BITS_SLICE(tx[2],0,7);
      // try to get info (find variant)
      const aocmd_osp_variant_t * var = 0;
      for( int vix=aocmd_osp_tidmap[tid].vix; vix<aocmd_osp_tidmap[tid].vix+aocmd_osp_tidmap[tid].num; vix++ ) {
        if( aocmd_osp_variant[vix].minsize<=payloadsize && payloadsize<=aocmd_osp_variant[vix].maxsize )
          var= &aocmd_osp_variant[vix];
      }
      if( var==0 ) var= &aocmd_osp_variant[aocmd_osp_tidmap[tid].vix]; // None fits; just pick first
      // subcommand
      if( argv[1][1]=='r' ) { // command "osp trx" (has rx)
        if( ! AOCMD_OSP_VARIANT_HAS_RESPONSE(var) ) Serial.printf("validate: a receive command is given, but %02X/%s has no response\n",var->tid,var->swname);
      } else { // command "osp tx" (tx only)
        if( AOCMD_OSP_VARIANT_HAS_RESPONSE(var) ) Serial.printf("validate: %02X/%s triggers response, but a tx only command is given\n",var->tid,var->swname);
      }
      // preamble
      if( preamble!=0xA ) Serial.printf("validate: first nibble should be preamble (0xA)\n");
      // addr
      if( ! AOOSP_ADDR_ISOK(addr) ) Serial.printf("validate: illegal addr %03X\n",addr);
      if( AOCMD_OSP_VARIANT_HAS_INFO(var) && AOOSP_ADDR_ISBROADCAST(addr) && !AOCMD_OSP_VARIANT_HAS_BROADMULTICAST(var) ) Serial.printf("validate: %02X/%s does not support broadcast\n",var->tid,var->swname);
      if( AOCMD_OSP_VARIANT_HAS_INFO(var) && OAOSP_ADDR_ISMULTICAST(addr) && !AOCMD_OSP_VARIANT_HAS_BROADMULTICAST(var) ) Serial.printf("validate: %02X/%s does not support multicast\n",var->tid,var->swname);
      // psi (payloadsize)
      if( AOCMD_OSP_VARIANT_HAS_INFO(var) && ( payloadsize < var->minsize  ||  var->maxsize < payloadsize ) ) {
        Serial.printf("validate: %02X/%s does not have %d bytes as payload, but",var->tid,var->swname,payloadsize);
        for( int vix=aocmd_osp_tidmap[tid].vix; vix<aocmd_osp_tidmap[tid].vix+aocmd_osp_tidmap[tid].num; vix++ )
          Serial.printf(" %d..%d", aocmd_osp_variant[vix].minsize, aocmd_osp_variant[vix].maxsize );
        Serial.printf("\n");
      }
      if( payloadsize<0 || payloadsize>8 || payloadsize==5 || payloadsize==7 ) Serial.printf("validate: illegal payload size %d (allowed is 0,1,2,3,4,6,8)\n", payloadsize);
      else if( PSI(payloadsize)!=psi  ) Serial.printf("validate: payload is %d bytes so psi should be %d but is %d \n", payloadsize,PSI(payloadsize),psi);
      // tid
      if( ! AOCMD_OSP_VARIANT_HAS_INFO(var) ) Serial.printf("validate: no info %02X/??? to validate against\n",var->tid);
      // crc
      if( aoosp_crc(tx,telesize-1)!=tx[telesize-1] ) Serial.printf("validate: crc %02X is incorrect (should be %02X)\n",tx[telesize-1],aoosp_crc(tx,telesize-1));
    }
  }

  if( argv[0][0]!='@' ) Serial.printf("tx %s\n", aoosp_prt_bytes(tx,telesize) );

  // Execute
  uint8_t rx[AOSPI_TELE_MAXSIZE];
  memset(rx,0xA5,AOSPI_TELE_MAXSIZE);
  aoresult_t result;
  if( argv[1][1]=='r' ) { // command "osp trx"
    int actsize;
    result = aospi_txrx(tx, telesize, rx, AOSPI_TELE_MAXSIZE, &actsize);
    Serial.printf("rx %s",aoosp_prt_bytes(rx,actsize));
  } else { // command "osp tx"
    result = aospi_tx(tx, telesize);
    Serial.printf("rx none");
  }
  Serial.printf(" %s\n",aoresult_to_str(result));
}


// Parse 'osp resetinit'
static void aocmd_osp_resetinit( int argc, char * argv[] ) {
  if( argc!=2 ) { Serial.printf("ERROR: too many arguments\n"); return; }

  uint16_t last; int loop;
  aoresult_t result = aoosp_exec_resetinit(&last,&loop);
  if(result!=aoresult_ok) { Serial.printf("ERROR: resetinit failed (%s)\n", aoresult_to_str(result) ); return; }
  if( argv[0][0]!='@' ) Serial.printf("resetinit: %s %03X (%s)\n", (loop?"loop":"bidir"), last, aoresult_to_str(result) );
}


// Parse 'osp enum'
static void aocmd_osp_enum( int argc, char * argv[] ) {
  if( argc!=2 ) { Serial.printf("ERROR: too many arguments\n"); return; }

  uint16_t last; int loop;
  aoresult_t result = aoosp_exec_resetinit(&last,&loop);
  if(result!=aoresult_ok) { Serial.printf("ERROR: resetinit failed (%s)\n", aoresult_to_str(result) ); return; }
  
  // Scan all OSP nodes
  int triplets=0;
  int i2cbridges=0;
  for( int addr=1; addr<=last; addr++ ) {
    // Print sio1 comm
    uint8_t  com;
    result = aoosp_send_readcomst(addr, &com );
    if( result!=aoresult_ok ) { Serial.printf("comst %s\n", aoresult_to_str(result) ); break; }
    Serial.printf("%4s", aoosp_prt_com_sio1(com) );
    // Print addr and id
    uint32_t id;
    result = aoosp_send_identify(addr, &id );
    if( result!=aoresult_ok ) { Serial.printf("ERROR: aoosp_send_identify(%03X) failed (%s)\n", addr, aoresult_to_str(result) ); return; }
    Serial.printf(" N%03X %08lX",addr,id);
    // Print name
    if( AOOSP_IDENTIFY_IS_SAID(id) ) {
      Serial.printf("/SAID T%d T%d", triplets, triplets+1);
      // Is this SAID having I2C bridge enabled?
      int enable;
      result = aoosp_exec_i2cenable_get(addr, &enable);
      if(result!=aoresult_ok) { Serial.printf("ERROR: aoosp_exec_i2cenable_get(%03X) failed (%s)\n", addr, aoresult_to_str(result) ); return; }
      if( enable ) {
        Serial.printf(" I%d", i2cbridges); 
        triplets+=2; 
        i2cbridges+=1;
      } else {
        Serial.printf(" T%d", triplets+2);
        triplets+=3;
      }
    } else if( AOOSP_IDENTIFY_IS_RGBI(id) ) {
      Serial.printf("/RGBI T%d", triplets);
      triplets += 1;
    } else {
      Serial.printf("/OTHER");
    }
    Serial.printf(" %s", aoosp_prt_com_sio2(com) );
    Serial.printf("\n");
  }
  // summary
  Serial.printf("nodes(N) 1..%d, ", last );
  Serial.printf("triplets(T) 0..%d, ", triplets-1 );
  if( i2cbridges == 0 ) 
    Serial.printf("i2cbridges(I) none, " );
  else
    Serial.printf("i2cbridges(I) 0..%d, ", i2cbridges-1 );
  Serial.printf("dir %s\n", loop?"loop":"bidir");
}


// The handler for the "osp" command
static void aocmd_osp_main( int argc, char * argv[] ) {
  AORESULT_ASSERT( aocmd_osp_tidmap[0x7F].vix!=0 ); // init has run 
  if( argc==1 ) {
    aocmd_osp_dirmux_show();
    aocmd_osp_validate_show();
    aocmd_osp_count_show();
    aocmd_osp_log_show(); 
  } else if( aocmd_cint_isprefix("dirmux",argv[1]) ) {
    if( argc==2 ) { aocmd_osp_dirmux_show(); return; }
    if( argc!=3 ) { Serial.printf("ERROR: 'dirmux' has too many args\n"); return; }
    if( aocmd_cint_isprefix("bidir",argv[2]) ) aospi_dirmux_set_bidir();
    else if( aocmd_cint_isprefix("loop",argv[2]) ) aospi_dirmux_set_loop();
    else { Serial.printf("ERROR: 'dirmux' must have 'bidir' or 'loop'\n"); return; }
    if( argv[0][0]!='@' ) aocmd_osp_dirmux_show();
  } else if( aocmd_cint_isprefix("validate",argv[1]) ) {
    if( argc==2 ) { aocmd_osp_validate_show(); return; }
    if( argc!=3 ) { Serial.printf("ERROR: 'validate' has too many args\n"); return; }
    if( aocmd_cint_isprefix("enable",argv[2]) ) oacmd_osp_validate=1;
    else if( aocmd_cint_isprefix("disable",argv[2]) ) oacmd_osp_validate=0;
    else { Serial.printf("ERROR: 'validate' must have 'enable' or 'disable'\n"); return; }
    if( argv[0][0]!='@' ) aocmd_osp_validate_show();
  } else if( aocmd_cint_isprefix("hwtest",argv[1]) ) {
    if( argc==2 ) { aocmd_osp_hwtestout_show(); aocmd_osp_hwtestin_show(); return; }
    if( argc>4 ) { Serial.printf("ERROR: 'hwtest' has too many args\n"); return; }
    if( aocmd_cint_isprefix("out",argv[2]) ) {
      if( argc==3 ) { aocmd_osp_hwtestout_show(); return; }
      if( aocmd_cint_isprefix("enable",argv[3]) ) aospi_outoena_set(HIGH);
      else if( aocmd_cint_isprefix("disable",argv[3]) ) aospi_outoena_set(LOW);
      else { Serial.printf("ERROR: 'hwtest out' must have 'enable' or 'disable', not '%s'\n",argv[3]); return; }
      if( argv[0][0]!='@' ) aocmd_osp_hwtestout_show();
    } else if( aocmd_cint_isprefix("in",argv[2]) ) {
      if( argc==3 ) { aocmd_osp_hwtestin_show(); return; }
      if( aocmd_cint_isprefix("enable",argv[3]) ) aospi_inoena_set(HIGH);
      else if( aocmd_cint_isprefix("disable",argv[3]) ) aospi_inoena_set(LOW);
      else { Serial.printf("ERROR: 'hwtest in' must have 'enable' or 'disable', not '%s'\n",argv[3]); return; }
      if( argv[0][0]!='@' ) aocmd_osp_hwtestin_show();
    } else { Serial.printf("ERROR: expected 'out' or 'in', not '%s'\n", argv[2]); return; }
  } else if( aocmd_cint_isprefix("count",argv[1]) ) {
    if( argc==2 ) { aocmd_osp_count_show(); return; }
    if( argc!=3 ) { Serial.printf("ERROR: 'count' has too many args\n"); return; }
    if( aocmd_cint_isprefix("reset",argv[2]) ) { /*nothing */ }
    else { Serial.printf("ERROR: 'count' may have 'reset' (not '%s')\n", argv[2]); return; }
    aospi_txcount_reset();
    aospi_rxcount_reset();
    if( argv[0][0]!='@' ) aocmd_osp_count_show();
  } else if( aocmd_cint_isprefix("log",argv[1]) ) {
    if( argc==2 ) { aocmd_osp_log_show(); return; }
    if( argc!=3 ) { Serial.printf("ERROR: 'log' has too many args\n"); return; }
    aoosp_loglevel_t level;
    if( aocmd_cint_isprefix("none",argv[2]) ) level= aoosp_loglevel_none;
    else if( aocmd_cint_isprefix("args",argv[2]) ) level= aoosp_loglevel_args;
    else if( aocmd_cint_isprefix("tele",argv[2]) ) level= aoosp_loglevel_tele;
    else { Serial.printf("ERROR: 'log' has 'none', 'args', or 'tele' (not '%s')\n", argv[2]); return; }
    aoosp_loglevel_set(level);
    if( argv[0][0]!='@' ) aocmd_osp_log_show();
  } else if( aocmd_cint_isprefix("info",argv[1]) ) {
    if( argc==2 ) { aocmd_osp_info_show(); return; }
    if( argc!=3 ) { Serial.printf("ERROR: 'info' has too many args\n"); return; }
    // todo: add sub-command to search in descriptions?
    #define LIST_FINDMAX 9
    int variants[LIST_FINDMAX];
    int found = aocmd_osp_variant_find( argv[2], variants, LIST_FINDMAX);
    if( found==0 ) { Serial.printf("ERROR: no <tele> found ('%s')\n", argv[2]); return; }
    int list= found==LIST_FINDMAX ? found-1 : found;
    for( int i=0; i<list; i++ ) aocmd_osp_variant_print( &aocmd_osp_variant[variants[i]] );
    if( found!=list ) { Serial.printf("WARNING: too many matches (list truncated)\n"); return; }
  } else if( aocmd_cint_isprefix("resetinit",argv[1]) ) {
    aocmd_osp_resetinit(argc, argv);
  } else if( aocmd_cint_isprefix("enum",argv[1]) ) {
    aocmd_osp_enum(argc, argv);
  } else if( aocmd_cint_isprefix("send",argv[1]) ) {
    aocmd_osp_send(argc, argv);
  } else if( aocmd_cint_isprefix("tx",argv[1]) || aocmd_cint_isprefix("trx",argv[1])) {
    aocmd_osp_trx(argc, argv);
  } else {
    Serial.printf("ERROR: unknown argument ('%s')\n", argv[1]); return;
  }
}


// The long help text for the "osp" command.
static const char aocmd_osp_longhelp[] =
  "SYNTAX: osp\n"
  "- shows mux, validate, and count status\n"
  "SYNTAX: osp dirmux [ bidir | loop ]\n"
  "- without optional argument shows the status of the direction mux\n"
  "- with optional argument sets the direction mux to bi-directional or loop\n"
  "SYNTAX: osp validate [enable|disable`]\n"
  "- without optional argument shows the status of telegram validation\n"
  "- with optional argument sets it\n"
  "- this validates (checks consistency) telegrams entered with 'send'/'tx'\n"
  "- enabled is slower, but invalid telegrams will be sent anyhow\n"
  "SYNTAX: osp hwtest (out|in) [enable|disable]\n"
  "- hardware test for the output enable of the out and in control lines\n"
  "- without optional argument shows the status of a line\n"
  "- with optional argument sets it\n"
  "- this is for testing only; these lines control two signalling LEDs on OSP32\n"
  "SYNTAX: osp count [ reset ]\n"
  "- without optional argument shows how many telegrams were sent and received\n"
  "- with 'reset', resets counters to 0\n"
  "- this is a count of SPI transactions (including failed ones)\n"
  "SYNTAX: osp log [ none | args | tele ]\n"
  "- without optional argument shows log status, with argument sets it\n"
  "- logs nothing, telegram name with args, or even raw telegram bytes\n"
  "- this is logs calls to the osp library, not the spi library used by 'osp'\n"
  "SYNTAX: osp info [ <tele> ]\n"
  "- without optional arguments lists all (known) telegrams\n"
  "- with argument, gives info on telegrams with <tele> in name (max 8)\n"
  "SYNTAX: osp resetinit\n"
  "- resetinit tries reset-initloop, then reset-initbidir (controls dirmux)\n"
  "SYNTAX: osp enum\n"
  "- enumerates all nodes in the chain (starts with resetinit)\n"
  "SYNTAX: osp send <addr> <tele> <data>...\n"
  "- this is a high level send, with auto-fill for pre-amble, PSI, CRC\n"
  "- sends telegram <tele> to node <addr> with optional <data>\n"
  "- if the <tele> has a response (see info), waits for and prints response\n"
  "- 'osp send 001 initbidir' and 'osp send 001 02' both send A0 04 02 A9\n"
  "SYNTAX: osp (tx|trx) <data>... [crc]\n"
  "- this is a low level send, pass pre-amble, PSI, CRC explicitly\n"
  "- with 'crc' computes crc and appends that to telegram\n"
  "- with 'tx' sends telegram consisting of all <data> bytes\n"
  "- with 'trx' also receives the response\n"
  "- note that a 'c' as last <data> is treated as crc not as 0C\n"
  "- 'osp tx A0 00 05 B1' and 'osp tx A0 00 05 crc' are 'osp send 000 goactive'\n"
  "NOTES:\n"
  "- some commands use leading @ to suppress output\n"
  "- <addr> is a node address in hex (1..3EA, 0 for broadcast, 3Fx for group)\n"
  "- <tele> is either a 2 digit hex number, or a (partial) telegram name\n"
  "- <data> is a (one-byte) argument in hex 00..FF\n"
;


/*!
    @brief  Registers the built-in "osp" command with the command interpreter.
    @return Number of remaining registration slots (or -1 if registration failed).
    @note   The aocmd_init calls this function, so normal client code does not need to call it.
    @note   If client code overrides the default registration by implementing
            its own aocmd_register() then this function could be called from there.
*/
int aocmd_osp_register() {
  return aocmd_cint_register(aocmd_osp_main, "osp", "sends and receives OSP telegrams", aocmd_osp_longhelp);
}





  

