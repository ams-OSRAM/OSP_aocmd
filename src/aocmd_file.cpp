// aocmd_file.cpp - command handler for the "file" command, also implements a one-file file system.
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


#include <Arduino.h>        // Serial.print
#include <esp32-hal-cpu.h>  // esp_reset_reason(), ESP_RST_POWERON
#include <EEPROM.h>         // BEGIN()
#include <aoresult.h>       // AORESULT_ASSERT
#include <aocmd_cint.h>     // aocmd_cint_register, aocmd_cint_isprefix, ...
#include <aocmd_file.h>     // own


// === aocmd_file_bootcmd.h ================================================


// The boot.cmd file contains a string; it is stored in EEPROM, with terminating 0. 
// This is the amount of EEPROM space reserved for it. One byte is appended, a check sum.
#define AOCMD_FILE_BOOTCMD_MAXSIZE  2047         


// Executes commands in boot.cmd on POR (encapsulates next three).
void aocmd_file_bootcmd_exec_on_por();
// Returns true iff the ESP was booted from Power On Reset.
static bool aocmd_file_bootcmd_reset_is_por();             
// Returns true iff boot.cmd is not empty and not corrupt (checksum).
static bool aocmd_file_bootcmd_available();             
// Executes commands in boot.cmd.
static void aocmd_file_bootcmd_exec();

// Computes checksum of boot.cmd.
static uint8_t aocmd_file_bootcmd_checksum();
                                                 
// Opens boot.cmd for reading; sets pointer at start.                                                 
static void aocmd_file_bootcmd_readopen();              
// Returns byte at pointer, or -1 for eof. Steps pointer.
static int  aocmd_file_bootcmd_readbyte();              
// Closes boot.cmd for reading.
static void aocmd_file_bootcmd_readclose();             

// Opens boot.cmd for writing; clears it; sets pointer at start.
static void aocmd_file_bootcmd_writeopen();             
// Writes byte to pointer. Steps pointer. Returns true iff success, false on file too long.
static bool aocmd_file_bootcmd_writebyte(uint8_t byte); 
// Returns -1 if commit fails, otherwise returns number of bytes written.
static int  aocmd_file_bootcmd_writeclose();            


// === aocmd_file_bootcmd.cpp ===============================================


#define AOCMD_FILE_BOOTCMD_STARTADDR_DATA     0
#define AOCMD_FILE_BOOTCMD_STARTADDR_CSUM     AOCMD_FILE_BOOTCMD_MAXSIZE 
#define AOCMD_FILE_BOOTCMD_EEPROMSIZE         (AOCMD_FILE_BOOTCMD_MAXSIZE+1) // checksum is one byte


// The pointer into boot.cmd file.
static int  aocmd_file_bootcmd_ptr;


/*!
    @brief  Initializes the persistent ESP file system (EEPROM based).
    @note   Prints to Serial upon failure.
*/
void aocmd_file_init() {
  AORESULT_ASSERT( EEPROM.length()==0 ); // Not yet inited
  bool ok = EEPROM.begin(AOCMD_FILE_BOOTCMD_EEPROMSIZE);
  AORESULT_ASSERT( EEPROM.length()==AOCMD_FILE_BOOTCMD_EEPROMSIZE ); // inited
  if( !ok) Serial.printf("file: init FAILED\n");
}


/*!
    @brief  Executes the file "boot.cmd" on power on reset, 
            by feeding its content to the command interpreter.
    @note   Before calling this function, make sure all libraries 
            are initialized (there might be commands that use those libraries),
            in particular make sure Serial and aocmd are initialized.
    @note   "boot.cmd" is stored in the persistent ESP filesystem.
    @note   If there is no "boot.cmd" this message is printed to Serial.
    @note   If the ESP resarted, but not from POR, this message is printed to Serial.
    @note   If "boot.cmd" is executed, this message is printed to Serial.
*/
void aocmd_file_bootcmd_exec_on_por() {
  AORESULT_ASSERT( EEPROM.length()==AOCMD_FILE_BOOTCMD_EEPROMSIZE ); // inited
  if( !aocmd_file_bootcmd_available() ) {
    Serial.printf("No 'boot.cmd' file available to execute\n");
    return;
  }
  
  if( ! aocmd_file_bootcmd_reset_is_por() ) {
    Serial.printf("Only power-on-reset runs 'boot.cmd'\n");
    return;
  }

  Serial.printf("Running 'boot.cmd'\n");
  aocmd_file_bootcmd_exec();
}


// Returns true iff the ESP was booted from Power On Reset.
static bool aocmd_file_bootcmd_reset_is_por() {
  return esp_reset_reason()==ESP_RST_POWERON;
}


// Returns true iff boot.cmd is not empty and not corrupt (checksum).
static bool aocmd_file_bootcmd_available() {
  bool empty = EEPROM.read(AOCMD_FILE_BOOTCMD_STARTADDR_DATA)==0; // terminating 0 as first byte
  bool csumok = aocmd_file_bootcmd_checksum() == EEPROM.read(AOCMD_FILE_BOOTCMD_STARTADDR_CSUM);
  return !empty && csumok;
}


// Executes commands in boot.cmd.
static void aocmd_file_bootcmd_exec() {
  if( !aocmd_file_bootcmd_available() ) { Serial.printf("file: 'boot.cmd' empty\n"); return; }
  aocmd_file_bootcmd_readopen();
  aocmd_cint_prompt(); // Print a prompt for the first line of the script
  int ch; while( (ch=aocmd_file_bootcmd_readbyte()) >= 0 ) aocmd_cint_add(ch);
  if( aocmd_cint_pendingschars()>0 ) aocmd_cint_add('\n');
  Serial.printf("\n\n"); // white line after final >>
  aocmd_file_bootcmd_readclose();
}


// Computes checksum of boot.cmd.
static uint8_t aocmd_file_bootcmd_checksum() {
  uint8_t csum=0xA5; // some "random" value which is not all 0s or all 1s.
  for( int ptr=0; ptr<AOCMD_FILE_BOOTCMD_MAXSIZE; ptr++ ) {
    uint8_t byte = EEPROM.read(AOCMD_FILE_BOOTCMD_STARTADDR_DATA+ptr);
    if( byte==0 ) break; // found terminating 0
    csum += byte;
  }
  return csum;
}


// Opens boot.cmd for reading; sets pointer at start.                                                 
static void aocmd_file_bootcmd_readopen() {
  aocmd_file_bootcmd_ptr = 0;
}


// Returns byte at pointer, or -1 for eof. Steps pointer.
static int aocmd_file_bootcmd_readbyte() {
  if( aocmd_file_bootcmd_ptr==AOCMD_FILE_BOOTCMD_MAXSIZE ) return -1;
  uint8_t byte = EEPROM.read(AOCMD_FILE_BOOTCMD_STARTADDR_DATA+aocmd_file_bootcmd_ptr);
  if( byte==0 ) return -1;
  aocmd_file_bootcmd_ptr++;
  return byte;
}


// Closes boot.cmd for reading.
static void aocmd_file_bootcmd_readclose() {
  // skip
}


// Opens boot.cmd for writing; clears it; sets pointer at start.
static void aocmd_file_bootcmd_writeopen() {
  // While writing, aocmd_file_bootcmd_ptr always points to first free position.
  aocmd_file_bootcmd_ptr = 0;
}


// Writes byte to pointer. Steps pointer. Returns true iff success, false on file too long.
static bool aocmd_file_bootcmd_writebyte(uint8_t byte) {
  if( aocmd_file_bootcmd_ptr==AOCMD_FILE_BOOTCMD_MAXSIZE-1 ) return false;
  // While writing, aocmd_file_bootcmd_ptr always points to first free position.
  EEPROM.write(AOCMD_FILE_BOOTCMD_STARTADDR_DATA+aocmd_file_bootcmd_ptr++, byte); 
  return true;
}


// Returns -1 if commit fails, otherwise returns number of bytes written.
static int aocmd_file_bootcmd_writeclose() {
  // While writing, aocmd_file_bootcmd_ptr always points to first free position.
  EEPROM.write(AOCMD_FILE_BOOTCMD_STARTADDR_DATA+aocmd_file_bootcmd_ptr, 0);
  uint8_t csum = aocmd_file_bootcmd_checksum();
  //Serial.printf("csum=0x%02x\n",csum);
  EEPROM.write(AOCMD_FILE_BOOTCMD_STARTADDR_CSUM,csum);
  if( !EEPROM.commit() ) return -1;
  return aocmd_file_bootcmd_ptr;
}


// === The actual "file" command ============================================


// file write streaming mode: line number
static int aocmd_file_write_linenum;


// file write streaming mode: streaming prompt
static void aocmd_file_write_setprompt() {
  char buf[16]; 
  aocmd_file_write_linenum++;
  snprintf(buf,sizeof buf, "%03d>> ",aocmd_file_write_linenum); 
  aocmd_cint_set_streamprompt(buf);  
}


// file write streaming mode: handler
static void aocmd_file_write_streamfunc( int argc, char * argv[] ) {
  if( argc==0 ) {
    // Input is a white line: save file and terminate streamin mode
    int size = aocmd_file_bootcmd_writeclose();
    if( size>=0 ) Serial.printf("file: %d bytes written\n",size); else Serial.printf("ERROR: save failed\n");
    aocmd_cint_set_streamfunc(0);
    return;
  }
  // Real line, append to file
  bool ok = true;
  for( int i=0; i<argc; i++ ) {
    if( i>0 ) ok &= aocmd_file_bootcmd_writebyte(' '); // space _between_ args
    char * s=argv[i];
    while( *s!=0 ) ok &= aocmd_file_bootcmd_writebyte(*s++);
  }
  ok &= aocmd_file_bootcmd_writebyte('\n'); // terminate line
  if( !ok ) { Serial.printf("ERROR: file too long\n"); return; }
  aocmd_file_write_setprompt();
}


// The handler for the "file" command
static void aocmd_file_main( int argc, char * argv[] ) {
  AORESULT_ASSERT( EEPROM.length()==AOCMD_FILE_BOOTCMD_EEPROMSIZE ); // inited
  if( argc>2 ) {
    Serial.printf("ERROR: too many arguments\n"); return;
  } else if( argc>1 && aocmd_cint_isprefix("show",argv[1])) {
    Serial.printf("file: 'boot.cmd' ");
    if( !aocmd_file_bootcmd_available() ) { Serial.printf("empty\n"); return; }
    Serial.printf("content:\n");
    aocmd_file_bootcmd_readopen();
    int ch; while( (ch=aocmd_file_bootcmd_readbyte()) >= 0 ) Serial.printf("%c",ch);
    aocmd_file_bootcmd_readclose();
  } else if( argc>1 && aocmd_cint_isprefix("exec",argv[1])) {
    aocmd_file_bootcmd_exec();
  } else if( argc>1 && aocmd_cint_isprefix("record",argv[1])) {
    aocmd_file_bootcmd_writeopen();
    aocmd_file_write_linenum=0;
    aocmd_file_write_setprompt();
    aocmd_cint_set_streamfunc(aocmd_file_write_streamfunc);
  } else {
    Serial.printf("ERROR: needs 'show', 'exec', or 'record'\n"); return;
  }
}


// The long help text for the "file" command.
static const char aocmd_file_longhelp[] = 
  "SYNTAX: file show\n"
  "- shows the content of the file (prints to console)\n"
  "SYNTAX: file exec\n"
  "- feed the content of file to the command interpreter (executes it)\n"
  "SYNTAX: file record <line>...\n"
  "- every <line> is written to the file\n"
  "- empty <line> stops streaming mode and commits content to file\n"
  "NOTES:\n"
  "- there is only one file (boot.cmd); it is run on cold startup\n"
  "- can make it empty with 'file record'\n"
;


/*!
    @brief  Registers the built-in "file" command with the command interpreter.
    @return Number of remaining registration slots (or -1 if registration failed).
    @note   The aocmd_init calls this function, so normal client code does not need to call it.
    @note   If client code overrides the default registration by implementing 
            its own aocmd_register() then this function could be called from there.
*/
int aocmd_file_register() {
  return aocmd_cint_register(aocmd_file_main, "file", "manages the file 'boot.cmd' with commands run at startup", aocmd_file_longhelp);
}

