// aocmd_echo.cpp - command handler for the "echo" command
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


#include <Arduino.h>        // Serial.print
#include <aocmd_cint.h>     // aocmd_cint_register, aocmd_cint_isprefix, ...
#include <aocmd_echo.h>     // own


//---------------------------------------------------------------------------
// This is a friend module of aocmd_cint. 
// It needs access to these private parts


extern bool aocmd_cint_echo; // Command interpreter should echo incoming chars (needed by friend module oacmd_echo)


//---------------------------------------------------------------------------


// Helper to print the echo status.
static void aocmd_echo_print() { 
  Serial.print(F("echo: echoing ")); 
  Serial.println(aocmd_cint_echo?F("enabled"):F("disabled")); 
}


// The handler for the "echo" command
static void aocmd_echo_main(int argc, char * argv[]) {
  if( argc==1 ) {
    aocmd_echo_print();
    return;
  }
  if( argc==3 && aocmd_cint_isprefix(PSTR("faults"),argv[1]) && aocmd_cint_isprefix(PSTR("step"),argv[2]) ) {
    aocmd_cint_steperrorcount();
    if( argv[0][0]!='@') Serial.println(F("echo: faults: stepped")); 
    return;
  }
  if( argc==2 && aocmd_cint_isprefix(PSTR("faults"),argv[1]) ) {
    int n= aocmd_cint_geterrorcount();
    if( argv[0][0]!='@') { Serial.print(F("echo: faults: ")); Serial.println(n); }
    return;
  }
  if( argc==2 && aocmd_cint_isprefix(PSTR("enabled"),argv[1]) ) {
    aocmd_cint_echo= true;
    if( argv[0][0]!='@') aocmd_echo_print();
    return;
  }
  if( argc==2 && aocmd_cint_isprefix(PSTR("disabled"),argv[1]) ) {
    aocmd_cint_echo= false;
    if( argv[0][0]!='@') aocmd_echo_print();
    return;
  }
  if( argc==3 && aocmd_cint_isprefix(PSTR("wait"),argv[1]) ) {
    int ms;
    if( ! aocmd_cint_parse_dec(argv[2],&ms) ) { Serial.printf("ERROR: wait time\n"); return; }
    if( argv[0][0]!='@') { Serial.print(F("echo: wait: ")); Serial.println(ms); }
    delay(ms);
    return;
  }
  int start= 1;
  if( aocmd_cint_isprefix(PSTR("line"),argv[1]) ) { start=2; }
  // This tries to restore the command line (put spaces back on the '\0's)
  //char * s0=argv[start-1]+strlen(argv[start-1])+1;
  //char * s1=argv[argc-1];
  //for( char * p=s0; p<s1; p++ ) if( *p=='\0' ) *p=' ';
  //Serial.println(s0); 
  for( int i=start; i<argc; i++) { if(i>start) Serial.print(' '); Serial.print(argv[i]);  }
  Serial.println();
}


// The long help text for the "echo" command.
static const char aocmd_echo_longhelp[] PROGMEM = 
  "SYNTAX: echo [line] <word>...\n"
  "- prints all words (useful in scripts)\n"
  "SYNTAX: echo faults [step]\n"
  "- without argument, shows and resets error counter\n"
  "- with argument 'step', steps the error counter\n"
  "- typically used for communication faults (serial rx buffer overflow)\n"
  "SYNTAX: echo [ enabled | disabled ]\n"
  "- with arguments enables/disables terminal echoing\n"
  "- (disabled is useful in scripts; output is relevant, but input much less)\n"
  "- without arguments shows status of terminal echoing\n"
  "SYNTAX: echo wait <time>\n"
  "- waits <time> ms (might be useful in scripts)\n"
  "NOTES:\n"
  "- supports @-prefix to suppress output\n"
  "- 'echo line' prints a white line (there are no <word>s)\n"
  "- 'echo line faults' prints 'faults'\n"
  "- 'echo line enabled' prints 'enabled'\n"
  "- 'echo line disabled' prints 'disabled'\n"
  "- 'echo line line' prints 'line'\n"
;


/*!
    @brief  Registers the built-in "echo" command with the command interpreter.
    @return Number of remaining registration slots (or -1 if registration failed).
    @note   The aocmd_init calls this function, so normal client code does not need to call it.
    @note   If client code overrides the default registration by implementing 
            its own aocmd_register() then this function could be called from there.
*/
int aocmd_echo_register() {
  return aocmd_cint_register(aocmd_echo_main, PSTR("echo"), PSTR("echo a message (or en/disables echoing)"), aocmd_echo_longhelp);
}

