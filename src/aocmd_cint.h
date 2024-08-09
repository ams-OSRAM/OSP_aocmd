// aocmd_cint.h - command interpreter (over UART/USB)
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
#ifndef _AOCMD_CINT_H_
#define _AOCMD_CINT_H_


// For AVR (not relevant for ESP8266/ESP32)
// Recall that F(xxx) puts literal xxx in PROGMEM _and_ makes it printable.
// It uses PSTR() to map it to PROGMEM, and f() to make it printable. Basically F(x) is f(PSTR(x))
// For some reason, Arduino defines PSTR() and F(), but not f().
// The macro f(xxx) assumes xxx is already a pointer to chars in PROGMEM' it only makes it printable.
#ifndef f
#define f(s) ((__FlashStringHelper *)(s)) // An empty class, so that the overloaded print() uses print-from-progmem
#endif


// The maximum number of characters the interpreter can buffer.
// The buffer is cleared when executing a command. Execution happens when a <CR> or <LF> is passed.
#define AOCMD_CINT_BUFSIZE 128
// When a command starts executing, it is split in arguments.
#define AOCMD_CINT_MAXARGS 32
// Total number of registration slots.
#define AOCMD_CINT_REGISTRATION_SLOTS 20
// Size of buffer for the streaming prompt
#define AOCMD_CINT_PROMPT_SIZE 10 
// Size of buffer for aocmd_cint_prt
#define AOCMD_CINT_PRT_SIZE 80 


// A command must implement a 'main' function. It is much like C's main, it has argc and argv.
typedef void (*aocmd_cint_func_t)( int argc, char * argv[] );
// To register a command pass a pointer to its 'main', the string 'name' that invokes it, and a short and long help text.
// All strings must be stored in PROGMEM (either using 'PSTR("help")' or 'const char s[] PROGMEM = "help";'.
// Registering returns -1 when registration failed. Can even be called before aocmd_cint_init()
int aocmd_cint_register(aocmd_cint_func_t main, const char * name, const char * shorthelp, const char * longhelp);  


// Initializes the command interpreter.
void aocmd_cint_init();
// Print the prompt when waiting for input (special variant when in streaming mode). Needed once after init().
void aocmd_cint_prompt();
// Add characters to the state machine of the command interpreter (firing a command on <CR>).
// Suggested to use aocmd_cint_pollserial(), which reads chars from Serial and calls aocmd_cint_add().
void aocmd_cint_add(int ch); 
// Convenient for automatic testing of command line processing. Add all characters of a string (don't forget the \n).
void aocmd_cint_addstr(const char * str); 
// Same as aocmd_cint_addstr(), but str in PROGMEN
void aocmd_cint_addstr_P(/*PROGMEM*/const char * str); 
// Returns the number of (not yet executed) chars.
int  aocmd_cint_pendingschars(); 


// The command handler can support streaming: sending data without commands. 
// To enable streaming, a command must install a streaming function f with aocmd_cint_set_streamfunc(f).
// Streaming is disabled via aocmd_cint_set_streamfunc(0).
void aocmd_cint_set_streamfunc(aocmd_cint_func_t func);
// Check which streaming function is installed (0 for none).
aocmd_cint_func_t aocmd_cint_get_streamfunc(void);
// Default prompt is >>, but when streaming is enabled a different prompt will be printed.
// Note 'prompt' will be copied to internal aocmd_cint_buf[AOCMD_CINT_BUFSIZE].
// Calling aocmd_cint_set_streamfunc(f) enables streaming; only when enabled, the streamprompt is used.
void aocmd_cint_set_streamprompt(const char * prompt);
// Get the streaming prompt.
const char * aocmd_cint_get_streamprompt(void);


// Helper functions


// Parse a string of a decimal number ("-12"). Returns false if there were errors. If true is returned, *v is the parsed value.
bool aocmd_cint_parse_dec(const char*s,int*v);
// Parse a string of a hex number ("0A8F"). Returns false if there were errors. If true is returned, *v is the parsed value.
bool aocmd_cint_parse_hex(const char*s,uint16_t*v) ;
// Returns true iff `prefix` is a prefix of `str`. Note `str` must be in PROGMEM (`prefix` in RAM)
bool aocmd_cint_isprefix(/*PROGMEM*/const char *str, const char *prefix);
// Reads Serial and calls aocmd_cint_add()
void aocmd_cint_pollserial( void );
// A print towards Serial, just like Serial.print, but now with formatting as printf()
int aocmd_cint_printf(const char *format, ...);
// A print towards Serial, just like Serial.print, but now with formatting as printf(), now from progmem
int aocmd_cint_printf_P(/*PROGMEM*/const char *format, ...);
// When aocmd_cint_pollserial() detects Serial buffer overflows it steps an error counter
void aocmd_cint_steperrorcount( void );
// The current error counter can be obtained with this function; as a side effect it clears the counter.
int  aocmd_cint_geterrorcount( void );


#endif
