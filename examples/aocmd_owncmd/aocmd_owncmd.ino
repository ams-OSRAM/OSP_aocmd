// aocmd_owncmd.ino - add own command to command interpreter
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
#include <aospi.h>          // aospi_init()
#include <aoosp.h>          // aoosp_init()
#include <aocmd.h>          // generic include for whole aocmd lib


/*
DESCRIPTION
This demo implements an application specific command, and registers it with 
the command interpreter. The new command has name "stat". It allows passing 
several (hex) numbers. The stat command keeps track of the count and the sum.

HARDWARE
The demo runs on the OSP32 board, no demo board needs to be attached.
In Arduino select board "ESP32S3 Dev Module".

BEHAVIOR
No commands are given to change the LEDs (in the below example).

OUTPUT
Welcome to aocmd_owncmd.ino
spi: init
osp: init
cmd: init

Type 'help' for help
Try 'stat 1 2 3' and 'stat show'
>> help stat
SYNTAX: stat reset
- resets the statistics counters
SYNTAX: stat show
- shows the statistics counters
SYNTAX: stat <hexnum>...
- updates the statistics counters (sum and count)
>> stat show
stat: 0/0
>> stat 2 2 2
>> stat show
stat: 6/3=2.00
>> stat 5 5
>> stat
stat: 16/5=3.20
>> stat reset
stat: reset
>> stat
stat: 0/0
>> 
*/


// --- new command "stat" ---------------------------------------------------
// This command allow passing several (hex) numbers.
// The stat command keeps track of the count and the sum.


// The statistics variables for the "stat" command
static int cmdstat_count=0;
static int cmdstat_sum=0;


// The command handler for the "stat" command
static void cmdstat_main(int argc, char * argv[]) {
  if( argc==2 && aocmd_cint_isprefix("reset",argv[1]) ) { 
    cmdstat_count= 0;
    cmdstat_sum= 0;
    Serial.printf("stat: reset\n");
    return;
  }
  if( argc==1 || (argc==2 && aocmd_cint_isprefix("show",argv[1])) ) {
    Serial.printf("stat: %d/%d", cmdstat_sum, cmdstat_count); 
    if( cmdstat_count>0 ) { 
      Serial.printf("=%0.2f\n",(float)cmdstat_sum/cmdstat_count); 
    } else {
      Serial.printf("\n");
    }
    return;
  }
  for( int i=1; i<argc; i++ ) {
    uint16_t val;
    bool ok= aocmd_cint_parse_hex(argv[i],&val) ;
    if( !ok ) {
      Serial.printf("ERROR: sum: value must be hex (is '%s')\n",argv[i]);
      return;
    }
    cmdstat_sum+= val;
    cmdstat_count+= 1;
  }
}


// The long help for the "stat" command
static const char cmdstat_longhelp[] = 
  "SYNTAX: stat reset\n"
  "- resets the statistics counters\n"
  "SYNTAX: stat show\n"
  "- shows the statistics counters\n"
  "SYNTAX: stat <hexnum>...\n"
  "- updates the statistics counters (sum and count)\n"
;


// Registers "stat" command
static void cmdstat_register() {
  aocmd_cint_register(cmdstat_main, "stat", "compute count, sum and average of hex numbers", cmdstat_longhelp);
}


// --------------------------------------------------------------------------


// Library aocmd "upcalls" via aocmd_register_extra() to allow the application to register extra commands.
void aocmd_register_extra() {
  cmdstat_register();
}


void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nWelcome to aocmd_owncmd.ino\n");

  aospi_init(); // used by command "osp" in aocmd
  aoosp_init(); // used by command "osp" in aocmd
  aocmd_init();
  
  aocmd_register(); // register all commands in aocmd lib
  cmdstat_register(); // register own command
  Serial.printf("\n");

  Serial.print( "Type 'help' for help\n" );
  Serial.print( "Try 'stat 1 2 3' and 'stat show'\n" );
  aocmd_cint_prompt();
}


void loop() {
  aocmd_cint_pollserial();
}

