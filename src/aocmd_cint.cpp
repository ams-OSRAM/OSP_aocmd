// aocmd_cint.cpp - command interpreter (over UART/USB)
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
// Recycled lib https://github.com/maarten-pennings/cmd version 8.1.0 to change prefix from cmd to aocmd_cint
// That library supports atmega range with limited memory, storing strings in flash using the F()/PSTR() macros.
// Where the original code uses these macros they're retained, newly written (handler) code (for ESP32) will not use them.


//#include <avr/pgmspace.h> // This library assumes most strings (command help texts) are in PROGMEM (flash, not RAM)
#include <Arduino.h>
#include <aocmd_cint.h>


// Some definitions are needed by a friend module (aocmd_echo, aocmd_help).
// These are tagged with FRIEND (instead of static).
#define FRIEND /*nothing*/


// A struct to store a command descriptor
FRIEND typedef struct aocmd_cint_desc_s { 
  aocmd_cint_func_t   main; 
  const char * name; 
  const char * shorthelp; 
  const char * longhelp; 
} aocmd_cint_desc_t;


// All command descriptors
FRIEND int aocmd_cint_descs_count= 0;
FRIEND aocmd_cint_desc_t aocmd_cint_descs[AOCMD_CINT_REGISTRATION_SLOTS];


// The registration function for command descriptors (all strings in PROGMEM!)
// Returns number of remaining free slots (or -1 and a Serial print if registration failed)
int aocmd_cint_register(aocmd_cint_func_t main, const char * name, const char * shorthelp, const char * longhelp) {
  // Is there still a free slot?
  if( aocmd_cint_descs_count >= AOCMD_CINT_REGISTRATION_SLOTS ) { Serial.printf("ERROR: command '%s' can not be registered (too many)\n",name); return -1; }
  int slot = aocmd_cint_descs_count;
  #if 1
    // command list is kept in alphabetical order
    while( slot>0 && strcmp(name,aocmd_cint_descs[slot-1].name)<0 ) {
      aocmd_cint_descs[slot] = aocmd_cint_descs[slot-1];
      slot--;
    }
  #endif
  aocmd_cint_descs_count++;
  
  aocmd_cint_descs[slot].main= main;
  aocmd_cint_descs[slot].name= name;
  aocmd_cint_descs[slot].shorthelp= shorthelp;
  aocmd_cint_descs[slot].longhelp= longhelp;
  
  return AOCMD_CINT_REGISTRATION_SLOTS - aocmd_cint_descs_count;
}


// Finds the command descriptor for a command with name `name`.
// When not found, returns 0.
FRIEND aocmd_cint_desc_t * aocmd_cint_find(char * name ) {
  if( aocmd_cint_descs_count==0 ) { Serial.printf("ERROR: no commands registered\n"); return 0; }
  for( int i=0; i<aocmd_cint_descs_count; i++ ) {
    if( aocmd_cint_isprefix(aocmd_cint_descs[i].name,name) ) return &aocmd_cint_descs[i];
  }
  return 0;
}


// The state machine for receiving characters via Serial
static char       aocmd_cint_buf[AOCMD_CINT_BUFSIZE];              // Incoming chars
static int        aocmd_cint_ix;                                   // Fill pointer into aocmd_cint_buf
FRIEND bool       aocmd_cint_echo;                                 // Command interpreter should echo incoming chars
static aocmd_cint_func_t aocmd_cint_streamfunc;                    // If 0, no streaming, else the streaming handler
static char       aocmd_cint_streamprompt[AOCMD_CINT_PROMPT_SIZE]; // If streaming (aocmd_cint_stream_main!=0), the streaming prompt


// Print the prompt when waiting for input (special variant when in streaming mode). Needed once after init().
void aocmd_cint_prompt() {
  if( aocmd_cint_streamfunc ) {
    Serial.print( aocmd_cint_streamprompt );
  } else {
    Serial.print( F(">> ") );  
  }
}


// Initializes the command interpreter.
void aocmd_cint_init() {
  aocmd_cint_ix= 0;
  aocmd_cint_echo= true;
  aocmd_cint_streamfunc= 0;
  aocmd_cint_streamprompt[0]= 0;
}


// Execute the entered command (terminated with a press on RETURN key)
static void aocmd_cint_exec() {
  char * argv[ AOCMD_CINT_MAXARGS ];
  // Cut a trailing comment
  char * cmt= strstr(aocmd_cint_buf,"//");
  if( cmt!=0 ) { *cmt='\0'; aocmd_cint_ix= cmt-aocmd_cint_buf; } // trim comment
  // Find the arguments (set up argv/argc)
  int argc= 0;
  int ix=0;
  while( ix<aocmd_cint_ix ) {
    // scan for begin of word (ie non-space)
    while( (ix<aocmd_cint_ix) && ( aocmd_cint_buf[ix]==' ' || aocmd_cint_buf[ix]=='\t' ) ) ix++;
    if( !(ix<aocmd_cint_ix) ) break;
    argv[argc]= &aocmd_cint_buf[ix];
    argc++;
    if( argc>AOCMD_CINT_MAXARGS ) { Serial.println(F("ERROR: too many arguments"));  return; }
    // scan for end of word (ie space)
    while( (ix<aocmd_cint_ix) && ( aocmd_cint_buf[ix]!=' ' && aocmd_cint_buf[ix]!='\t' ) ) ix++;
    aocmd_cint_buf[ix]= '\0';
    ix++;
  }
  //for(ix=0; ix<argc; ix++) { Serial.print(ix); Serial.print("='"); Serial.print(argv[ix]); Serial.print("'"); Serial.println(""); }
  // Check from streaming
  if( aocmd_cint_streamfunc ) {
    aocmd_cint_streamfunc(argc, argv); // Streaming mode is active pass the data
    return;
  }
  // Bail out when empty
  if( argc==0 ) {
    // Empty command entered
    return; 
  }
  // Find the command
  char * s= argv[0];
  if( *s=='@' ) s++;
  aocmd_cint_desc_t * d= aocmd_cint_find(s);
  // If a command is found, execute it 
  if( d!=0 ) {
    aocmd_cint_ix = 0; // Added because there might be a command that issues a command
    d->main(argc, argv ); // Execute handler of command
    return;
  } 
  Serial.print(F("ERROR: command '")); 
  Serial.print(s); 
  Serial.println(F("' not found (try help)")); 
}


// Add characters to the state machine of the command interpreter (firing a command on <CR>)
void aocmd_cint_add(int ch) {
  if( ch=='\n' || ch=='\r' ) {
    if( aocmd_cint_echo ) Serial.println();
    aocmd_cint_buf[aocmd_cint_ix]= '\0'; // Terminate (make aocmd_cint_buf a c-string)
    aocmd_cint_exec();
    aocmd_cint_ix=0;
    aocmd_cint_prompt(); // trigger for tests that cmd is finished
  } else if( ch=='\b' ) {
    if( aocmd_cint_ix>0 ) {
      if( aocmd_cint_echo ) Serial.print( F("\b \b") );
      aocmd_cint_ix--;
    } else {
      // backspace with no more chars in buf; ignore
    }
  } else {
    if( aocmd_cint_ix<AOCMD_CINT_BUFSIZE-1 ) {
      aocmd_cint_buf[aocmd_cint_ix++]= ch;
      if( aocmd_cint_echo ) Serial.print( (char)ch );
    } else {
      // Input buffer full, send "alarm" back, even with echo off
      Serial.print( F("_\b") ); // Prefer visual instead of \a (bell)
    }
  }
}


// Add all characters of a string (don't forget the \n)
void aocmd_cint_addstr(const char * str) {
  while( *str!='\0' ) aocmd_cint_add(*str++);  
}


// Add all characters of a string (don't forget the \n)
void aocmd_cint_addstr_P(/*PROGMEM*/const char * str) {
  char ch;
  while( '\0' != (ch=pgm_read_byte(str++)) ) aocmd_cint_add(ch);  
}


// Returns the number of (not yet executed) chars.
int aocmd_cint_pendingschars() {
  return aocmd_cint_ix;
}


// Helpers =========================================================================


void aocmd_cint_set_streamfunc(aocmd_cint_func_t func) {
  aocmd_cint_streamfunc= func;
}


aocmd_cint_func_t aocmd_cint_get_streamfunc(void) {
  return aocmd_cint_streamfunc;
}


void aocmd_cint_set_streamprompt(const char * prompt) {
  strncpy(aocmd_cint_streamprompt, prompt, AOCMD_CINT_PROMPT_SIZE);
  aocmd_cint_streamprompt[AOCMD_CINT_PROMPT_SIZE-1]= '\0';
}


const char * aocmd_cint_get_streamprompt(void) {
  return aocmd_cint_streamprompt;
}


// Parse a string of a hex number ("0A8F"), returns false if there were errors. 
// If true is returned, *v is the parsed value.
bool aocmd_cint_parse_hex(const char*s,uint16_t*v) {
  if( v==0 ) return false; 
  *v= 0;
  if( s==0 ) return false; // no string: not ok
  if( *s==0 ) return false; // empty string: not ok
  while( *s=='0' ) s++; // strip leading 0's
  if( strlen(s)>4 ) return false;
  while( *s!=0 ) {
    if     ( '0'<=*s && *s<='9' ) *v = (*v)*16 + *s - '0';
    else if( 'a'<=*s && *s<='f' ) *v = (*v)*16 + *s - 'a' + 10;
    else if( 'A'<=*s && *s<='F' ) *v = (*v)*16 + *s - 'A' + 10;
    else return false;
    s++;
  }
  return true;
}


// Parse a string of a decimal number ("-12"), returns false if there were errors. 
// If true is returned, *v is the parsed value.
bool aocmd_cint_parse_dec(const char*s,int*v) {
  if( v==0 ) return false; 
  *v= 0;
  if( s==0 ) return false; // no string: not ok
  if( *s==0 ) return false; // empty string: not ok
  int sign=+1;
  if( *s=='+' ) {
    s++;
    sign=+1;
  } else if( *s=='-' ) {
    s++;
    sign=-1;
  }
  if( *s==0 ) return false; // empty string after sign: not ok
  while( *s=='0' ) s++; // strip leading 0's
  while( *s!=0 ) {
    if( *s<'0' || '9'<*s ) return false;
    int vv= (*v)*10 + *s - '0';
    if( vv<*v ) return false; // overflow
    *v = vv;
    s++;
  }
  *v *= sign;
  return true;
}


// Returns true iff `prefix` is a prefix of `str`. Note `str` must be in PROGMEM (and `prefix` in RAM)
bool aocmd_cint_isprefix(const char *str, const char *prefix) {
  while( *prefix!='\0') {
    byte b= pgm_read_byte(str);
    if( b!=*prefix ) return false;
    str++;
    prefix++;
  }
  return true;
}


// A (formatting) printf towards Serial
// Note: to print string from PROGMEM use %S (capital S), and PSTR for the string (but F also works)
//   aocmd_cint_printf( "%S/%S\n", PSTR("foo"), F("bar") );
static char aocmd_cint_printf_buf[AOCMD_CINT_PRT_SIZE];
int aocmd_cint_printf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  int result = vsnprintf(aocmd_cint_printf_buf, AOCMD_CINT_PRT_SIZE, format, args);
  Serial.print(aocmd_cint_printf_buf);
  if( result>=AOCMD_CINT_PRT_SIZE ) Serial.print(F("\nOVERFLOW\n"));
  va_end(args);
  return result;
}


// A (formatting) printf towards Serial (the format string is in PROGMEM)
// Note: to print string from PROGMEM use %S (capital S), and PSTR for the string (but F also works). Format string must be PSTR()
//   aocmd_cint_printf_P( PSTR("%S/%S\n"), PSTR("foo"), F("bar") );
int aocmd_cint_printf_P(/*PROGMEM*/const char *format, ...) {
  va_list args;
  va_start(args, format);
  int result = vsnprintf_P(aocmd_cint_printf_buf, AOCMD_CINT_PRT_SIZE, format, args);
  Serial.print(aocmd_cint_printf_buf);
  if( result>=AOCMD_CINT_PRT_SIZE ) Serial.print(F("\nOVERFLOW\n"));
  va_end(args);
  return result;
}


// Steps the cmd error counter (observable via 'echo error')
static int aocmd_cint_errorcount= 0;
void aocmd_cint_steperrorcount( void ) {
  aocmd_cint_errorcount++;
}


// Returns and clears the cmd error counter
int aocmd_cint_geterrorcount( void ) {
  int current= aocmd_cint_errorcount;
  aocmd_cint_errorcount= 0;
  return current;
}


// Check Serial for incoming chars, and feeds them to the command handler.
// Flags buffer overflows via aocmd_cint_steperrorcount() - observable via 'echo error'
void aocmd_cint_pollserial( void ) {
  // Check incoming serial chars
  int n= 0; // Counts number of bytes read, this is roughly the number of bytes in the UART buffer
  while( 1 ) {
    int ch= Serial.read();
    if( ch==-1 ) break;
    if( 
#if defined(ESP8266) // #warning ESP8266
        Serial.hasOverrun()
#elif defined(ESP32) // #warning ESP32
        ++n==256 // Hack; default RC buffer size is 256, see HardwareSerial.cpp line 55: _uart = uartBegin(...256...);
#else // #warning AVR
        ++n==SERIAL_RX_BUFFER_SIZE // Possible UART buffer overrun
#endif
    ) {
      aocmd_cint_steperrorcount();
      Serial.println(); Serial.println( F("WARNING: serial overflow") ); Serial.println(); 
    }
    // Process read char by feeding it to command interpreter
    aocmd_cint_add(ch);
  }
}



