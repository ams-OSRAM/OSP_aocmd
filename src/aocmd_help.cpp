// aocmd_help.cpp - command handler for the "help" command
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
#include <aocmd_help.h>     // own


//---------------------------------------------------------------------------
// This is a friend module of aocmd_cint. 
// It needs access to these private parts


// A struct to store a command descriptor
typedef struct aocmd_cint_desc_s { 
  aocmd_cint_func_t   main; 
  const char * name; 
  const char * shorthelp; 
  const char * longhelp; 
} aocmd_cint_desc_t;


// All command descriptors (not static, because needed by friend module "help")
extern int aocmd_cint_descs_count;
extern aocmd_cint_desc_t aocmd_cint_descs[AOCMD_CINT_REGISTRATION_SLOTS];


// Finds the command descriptor for a command with name `name`.
// When not found, returns 0.
extern aocmd_cint_desc_t * aocmd_cint_find(char * name );


//---------------------------------------------------------------------------


// if verbose==0 only show section headers
// if topic==0 show all (SYNTAX) sections, otherwise show sections whose header contains `topic`
static void aocmd_help_showlonghelp(const char * longhelp, int verbose, const char * topic) {
  // longhelp is in PROGMEM so we need to get the chars one by one...
  //for(unsigned i=0; i<strlen_P(d->longhelp); i++) 
  //  Serial.print((char)pgm_read_byte_near(d->longhelp+i));

  const char * str= longhelp; // cursor
  int len= strlen_P(longhelp); // number of chars still to print
  int syntax= 0; // 1 when printing a line starting with "SYNTAX"
  int target= 1; // 1 when we had a "SYNTAX" line that matched topic
  while( len>0 ) {
    // Take next chunk of longhelp (into ram)
    #define SIZE 80 // long enough for one line
    char ram[SIZE+1]; // +1 for \0
    int size= len<SIZE ? len : SIZE;
    memcpy_P(ram, str, size);
    ram[size]='\0';
    // Break into lines
    char *nl = strchr(ram,'\n'); // 0 when there is no newline in ram
    if( nl!=0 ) { size= 1+nl-ram; ram[size]='\0'; } // clip after newline char
    // Check if chunk starts with SY (SYNTAX) - if line is longer than SIZE, don't reset `syntax`
    if( !syntax ) {
      // Is this a section?
      syntax= isupper(ram[0]); // SYNTAX, NOTES
      // Is this the target section
      if( syntax && topic ) target= strstr(ram,topic)!=0; 
    }
    if( syntax || verbose ) if( topic==0 || target ) Serial.print(ram);
    // If there was a newline, clear `syntax`
    if( nl!=0 ) syntax=0;
    str+= size;
    len-= size;
  }
}


// The handler for the "help" command
static void aocmd_help_main(int argc, char * argv[]) {
  if( argc==1 ) {
    if( argv[0][0]!='@' ) Serial.println(F("Available commands"));
    for( int i=0; i<aocmd_cint_descs_count; i++ ) {
      Serial.print(f(aocmd_cint_descs[i].name));
      Serial.print(' ');
      if( argv[0][0]!='@' ) {
        Serial.print('-');
        Serial.print(' ');
        Serial.println(f(aocmd_cint_descs[i].shorthelp));
      }
    }
    if( argv[0][0]=='@' ) Serial.println();
  } else if( argc==2 || argc==3 ) {
    aocmd_cint_desc_t * d= aocmd_cint_find(argv[1]);
    if( d==0 ) {
      Serial.println(F("ERROR: command not found (try 'help')"));    
    } else {
      aocmd_help_showlonghelp( d->longhelp, argv[0][0]!='@', argc==3?argv[2]:0 );
    }
  } else {
    Serial.println(F("ERROR: 'help' has too many args"));
  }
}


// The long help text for the "help" command.
static const char aocmd_help_longhelp[] PROGMEM = 
  "SYNTAX: help\n"
  "- lists all commands\n"
  "SYNTAX: help <cmd> [ <topic> ]\n"
  "- gives detailed help on command <cmd>\n"
  "- with <topic> show subset where section header contains <topic>\n"
  "NOTES:\n"
  "- supports @-prefix to suppress output\n"
  "NOTES:\n"
  "- all commands may be shortened, for example 'help', 'hel', 'he', 'h'\n"
  "- all sub commands may be shortened, for example 'help help' to 'help h'\n"
  "- normal prompt is >>, other prompt indicates streaming mode\n"
  "- commands may be suffixed with a comment starting with //\n"
  "- some commands support a @ as prefix; it suppresses output of that command\n"
;


/*!
    @brief  Registers the built-in "help" command with the command interpreter.
    @return Number of remaining registration slots (or -1 if registration failed).
    @note   The aocmd_init calls this function, so normal client code does not need to call it.
    @note   If client code overrides the default registration by implementing 
            its own aocmd_register() then this function could be called from there.
*/
int aocmd_help_register() {
  return aocmd_cint_register(aocmd_help_main, PSTR("help"), PSTR("gives help (try 'help help')"), aocmd_help_longhelp);
}
