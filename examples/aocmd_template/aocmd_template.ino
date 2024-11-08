// aocmd-appversion.ino - a template for an application with a command handler
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
#include "aocmd_template.h" // version info


/*
DESCRIPTION
This sketch is a template for an application with a command handler.
It includes an application banner, it implements the upcalls from 
the version command, and it runs boot.cmd on startup.

HARDWARE
The demo runs on the OSP32 board, no demo board needs to be attached.
In Arduino select board "ESP32S3 Dev Module".

BEHAVIOR
No commands are given to change the LEDs (in the below example).

OUTPUT
 _______                   _       _
|__   __|                 | |     | |
   | | ___ _ __ ___  _ __ | | __ _| |_ ___
   | |/ _ \ '_ ` _ \| '_ \| |/ _` | __/ _ \
   | |  __/ | | | | | |_) | | (_| | ||  __/
   |_|\___|_| |_| |_| .__/|_|\__,_|\__\___|
                    | |
                    |_|
Application template - version 2.2

spi: init
osp: init
cmd: init

No 'boot.cmd' file available to execute
Type 'help' for help
>> version
app     : Application template 2.2
runtime : Arduino ESP32 3_0_3
compiler: 12.2.0
arduino : 10607 (likely IDE2.x)
compiled: Sep  6 2024, 16:38:13
aolibs  : result 0.4.1 spi 0.5.1 osp 0.4.1 cmd 0.5.2
file    : aocmd_template.ino
>> 
*/


// Library aocmd "upcalls" via aocmd_version_app() to allow the application to print its version.
// If not implemented, version command prints "no application version registered\n".
void aocmd_version_app() {
  Serial.printf("%s %s\n", AOCMD_TEMPLATE_LONGNAME, AOCMD_TEMPLATE_VERSION );
}


// Library aocmd "upcalls" via aocmd_version_app() to allow the application to print extra version info
// Implementing this function is optional.
void aocmd_version_extra() {
  Serial.printf( "file    : %s\n", aoresult_shorten(__FILE__) ); // just a (silly) example
  // Serial.printf( "aolibs  : ui32 %s mw %s apps %s\n", AOUI32_VERSION, AOMW_VERSION, AOAPPS_VERSION);
}


void setup() {
  Serial.begin(115200);
  do delay(250); while( ! Serial );
  Serial.printf(AOCMD_TEMPLATE_BANNER);
  Serial.printf("%s - version %s\n\n", AOCMD_TEMPLATE_LONGNAME, AOCMD_TEMPLATE_VERSION);

  aospi_init();
  aoosp_init();
  aocmd_init();
  aocmd_register(); // register all commands in aocmd lib (option: add commands in aomw)
  Serial.printf("\n");

  aocmd_file_bootcmd_exec_on_por(); 
  Serial.printf( "Type 'help' for help\n" );
  aocmd_cint_prompt();
}


void loop() {
  aocmd_cint_pollserial();
  
  // ... other processing ...
}

