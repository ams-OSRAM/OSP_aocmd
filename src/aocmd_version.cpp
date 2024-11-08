// aocmd_version.cpp - command handler for the "version" command
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
#include <core_version.h>   // ARDUINO_ESP32_RELEASE
#include <aoresult.h>       // AORESULT_VERSION
#include <aospi.h>          // AOSPI_VERSION
#include <aoosp.h>          // AOOSP_VERSION
#include <aocmd.h>          // AOCMD_VERSION and own


/*!
    @brief  The version command prints the version of the various ingredients 
            that make up the application. This function is called by it; it 
            shall print to Serial the application name and version.
    @note   The version command handler calls aocmd_version_app(). The 
            implementation in this library prints an error. It is weakly 
            linked, so a client should itself implement aocmd_version_app().
*/
void __attribute__((weak)) aocmd_version_app() {
  Serial.printf( "no application version registered\n" );
}


/*!
    @brief  The version command prints the version of the various ingredients 
            that make up the application. This function is called by it; it 
            may print to Serial additional ingredients with name and version.
    @note   The version command handler calls aocmd_version_extra(). The 
            implementation in this library is empty. It is weakly linked, 
            so a client could itself implement aocmd_version_extra().
*/
void __attribute__((weak)) aocmd_version_extra() {
  // empty
}


// The handler for the "version" command
static void aocmd_version_main( int argc, char * argv[] ) {
  if( argc==1 ) {
    if( argv[0][0]!='@' ) Serial.printf( "app     : "); 
    aocmd_version_app();  
    if( argv[0][0]!='@' ) Serial.printf( "runtime : Arduino ESP32 " ARDUINO_ESP32_RELEASE "\n" );
    if( argv[0][0]!='@' ) Serial.printf( "compiler: " __VERSION__ "\n" );
    if( argv[0][0]!='@' ) Serial.printf( "arduino : %d%s\n",ARDUINO, (ARDUINO<10800?" (likely IDE2.x)":"") );
    if( argv[0][0]!='@' ) Serial.printf( "compiled: " __DATE__ ", " __TIME__ "\n" );
    if( argv[0][0]!='@' ) Serial.printf( "aolibs  : result %s spi %s osp %s cmd %s\n", AORESULT_VERSION, AOSPI_VERSION, AOOSP_VERSION, AOCMD_VERSION);
    if( argv[0][0]!='@' ) aocmd_version_extra();
    return;
  }
  Serial.printf("ERROR: 'version' has unknown argument ('%s')\n", argv[1]); return;
}


// The long help text for the "version" command.
static const char aocmd_version_longhelp[] = 
  "SYNTAX: version\n"
  "- lists version of this application, its libraries and tools to build it\n"
  "NOTES:\n"
  "- supports @-prefix to suppress output\n"
;


/*!
    @brief  Registers the built-in "version" command with the command interpreter.
    @return Number of remaining registration slots (or -1 if registration failed).
    @note   The aocmd_init calls this function, so normal client code does not need to call it.
    @note   If client code overrides the default registration by implementing 
            its own aocmd_register() then this function could be called from there.
*/
int aocmd_version_register() {
  return aocmd_cint_register(aocmd_version_main, "version", "version of this application, its libraries and tools to build it", aocmd_version_longhelp);
}

