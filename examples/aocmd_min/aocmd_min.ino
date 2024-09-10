// aocmd_min.ino - minimal command interpreter with just the built-in commands
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
This demo initializes the command interpreter, then starts
polling Serial for incoming characters. These are buffered until an
end-of-line is received (either CR or LF), then the command is parsed 
and executed. This demo does not add its own command.

HARDWARE
The demo runs on the OSP32 board, no demo board needs to be attached.
In Arduino select board "ESP32S3 Dev Module".

BEHAVIOR
No commands are given to change the LEDs (in the below example).

OUTPUT
Welcome to aocmd_min.ino
spi: init
osp: init
cmd: init

Type 'help' for help
>> help
Available commands
board - board info and commands
echo - echo a message (or en/disables echoing)
file - manages the file 'boot.cmd' with commands run at startup
help - gives help (try 'help help')
osp - sends and receives OSP telegrams
said - sends and receives SAID specific telegrams
version - version of this application, its libraries and tools to build it
>> echo Hello, world!
Hello, world!
>> @echo wait 5000
>> foo
ERROR: command 'foo' not found (try help)
>> help help
SYNTAX: help
- lists all commands
SYNTAX: help <cmd> [ <topic> ]
- gives detailed help on command <cmd>
- with <topic> show subset where section header contains <topic>
NOTES:
- supports @-prefix to suppress output
NOTES:
- all commands may be shortened, for example 'help', 'hel', 'he', 'h'
- all sub commands may be shortened, for example 'help help' to 'help h'
- normal prompt is >>, other prompt indicates streaming mode
- commands may be suffixed with a comment starting with //
- some commands support a @ as prefix; it suppresses output of that command
>> 
*/


void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nWelcome to aocmd_min.ino\n");

  aospi_init(); // used by command "osp" in aocmd
  aoosp_init(); // used by command "osp" in aocmd
  aocmd_init();
  aocmd_register(); // register all commands in aocmd lib
  Serial.printf("\n");

  Serial.println( "Type 'help' for help" );
  aocmd_cint_prompt(); // print initial prompt
}


void loop() {
  // Get chars from Serial, push them to the command interpreter.
  // If the char is <CR> or <LF>, parse and execute command, and print next prompt.
  aocmd_cint_pollserial();
}
