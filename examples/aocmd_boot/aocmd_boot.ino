// aocmd_boot.ino - testing boot.cmd, stored persistently on the ESP
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
This demo implements an application that prints a message every x ms.
It adds a command ("wait") to set and get x. This command can then be 
stored in boot.cmd, to configure this app persistently. The default
value of x is 30000 (30 seconds).

HARDWARE
The demo runs on the OSP32 board, no demo board needs to be attached.
In Arduino select board "ESP32S3 Dev Module".

BEHAVIOR
No commands are given to change the LEDs (in the below example).


OUTPUT
Welcome to aocmd_boot.ino
spi: init
osp: init
cmd: init

No 'boot.cmd' file available to execute
Type 'help' for help
Try 'wait 5000'
>> 
message (wait=30000 ms)
message (wait=30000 ms)
message (wait=30000 ms)
>> file record
001>> echo New wait
002>> wait 10000
003>> 
file: 25 bytes written
>> 
>> board reboot

... software triggered reboot ...

Welcome to aocmd_boot.ino
spi: init
osp: init
cmd: init

only power-on runs 'boot.cmd'
Type 'help' for help
Try 'wait 5000'
>> 

... pressed RST button ...

Welcome to aocmd_boot.ino
spi: init
osp: init
cmd: init

Running 'boot.cmd'
>> echo New wait
New wait
>> wait 10000
>> 

Type 'help' for help
Try 'wait 5000'
>>
message (wait=10000 ms)
message (wait=10000 ms)
message (wait=10000 ms)
*/


// --- command "wait" --------------------------------------------------------
// This command allow setting and getting a delay.


// The statistics variables for the "stat" command
static uint32_t cmdwait_ms = 30*1000;


// The command handler for the "wait" command
static void cmdwait_main(int argc, char * argv[]) {
  if( argc==1  ) { 
    Serial.printf("wait: %lu ms\n", cmdwait_ms);
    return;
  }
  if( argc==2 ) {
    int val;
    bool ok= aocmd_cint_parse_dec(argv[1],&val) ;
    if( !ok || val<0) { Serial.printf("ERROR: wait: value must be non-negative decimal '%s'\n",argv[1]); return; }
    cmdwait_ms= val;
    return;
  }
  Serial.printf("ERROR: wait: unknown argument\n");
}


// The long help for the "wait" command
static const char cmdwait_longhelp[] = 
  "SYNTAX: wait\n"
  "- shows the wait time (ms)\n"
  "SYNTAX: wait <time>\n"
  "- sets the wait time (ms)\n"
;


// Registers "wait" command
static void cmdwait_register() {
  aocmd_cint_register(cmdwait_main, "wait", "controls the wait time between a periodic message", cmdwait_longhelp);
}


// ---------------------------------------------------------------------------


static uint32_t last;


void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nWelcome to aocmd_boot.ino\n");

  aospi_init(); // used by command "osp" in aocmd
  aoosp_init(); // used by command "osp" in aocmd
  aocmd_init();
  aocmd_register(); // register all commands in aocmd lib
  cmdwait_register(); // register extra command 'wait'
  Serial.printf("\n");

  // Exec file boot.cmd on POR
  aocmd_file_bootcmd_exec_on_por(); 
  
  Serial.print( "Type 'help' for help\n" );
  Serial.print( "Try 'wait 5000'\n" );
  aocmd_cint_prompt(); 
  
  last = millis();
}


void loop() {
  // Processing of incoming commands
  aocmd_cint_pollserial();

  // Main application
  if( millis()-last > cmdwait_ms ) {
    Serial.printf("message (wait=%lu ms)\n",cmdwait_ms );
    last = millis();
  }
}

