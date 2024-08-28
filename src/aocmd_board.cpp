// aocmd_board.cpp - command handler for the "board" command
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
#include <esp32-hal-cpu.h>  // esp_reset_reason()
#include <esp_chip_info.h>  // esp_chip_info_t
#include <esp_flash.h>      // esp_flash_get_size()
#include <aoresult.h>       // AORESULT_ASSERT
#include <aocmd_cint.h>     // aocmd_cint_register, aocmd_cint_isprefix, ...
#include <aocmd_board.h>    // own


// If the top-level application needs more board info to be printed, 
// it should implement this function and print that to Serial.
void __attribute__((weak)) aocmd_board_extra() {
  // empty
}


static const char * aocmd_board_resetreason() {
  esp_reset_reason_t reason = esp_reset_reason(); 
  switch( reason ) {
    case ESP_RST_UNKNOWN   : return "unknown";
    case ESP_RST_POWERON   : return "power-on";
    case ESP_RST_EXT       : return "reset-external-pin";
    case ESP_RST_SW        : return "reset-by-sw";
    case ESP_RST_PANIC     : return "exception-or-panic";
    case ESP_RST_INT_WDT   : return "watchdog-interrupt";
    case ESP_RST_TASK_WDT  : return "watchdog-task";
    case ESP_RST_WDT       : return "watchdog-other";
    case ESP_RST_DEEPSLEEP : return "from-deepsleep";
    case ESP_RST_BROWNOUT  : return "brownout";
    case ESP_RST_SDIO      : return "SDIO";
    default                : return "<should-not-happen>";
  }
}


static void aocmd_board_clk_show() {
  Serial.printf( "clk  : %lu MHz (xtal %lu MHz)\n",getCpuFrequencyMhz(), getXtalFrequencyMhz() );
}


static void aocmd_board_show() {
  Serial.printf( "chip : model %s (%d cores) rev %d\n",ESP.getChipModel(),ESP.getChipCores(), ESP.getChipRevision() );
  aocmd_board_clk_show();
  Serial.printf( "ftrs :");
    esp_chip_info_t info;
    esp_chip_info(&info);
    if( info.features & BIT(0) ) Serial.printf(" Embedded-Flash");
    if( info.features & BIT(1) ) Serial.printf(" 2.4GHz-WiFi");
    if( info.features & BIT(4) ) Serial.printf(" Bluetooth-LE");
    if( info.features & BIT(5) ) Serial.printf(" Bluetooth-classic");
    Serial.printf( "\n");
  uint32_t flashsize; esp_flash_get_size(NULL,&flashsize);
  Serial.printf( "flash: %ld byte %s flash\n", flashsize, (info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
  Serial.printf( "app  : %lu byte\n", ESP.getSketchSize() );
  Serial.printf( "reset: %s\n",aocmd_board_resetreason() );
  aocmd_board_extra();
}


// Next function deliberately causes a stack overflow
// The pragma's suppress a warning for that
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winfinite-recursion"

static int aocmd_board_stackoverflow() {
  volatile int val = aocmd_board_stackoverflow(); // volatile suppresses optimizations to force stack usage
  return val;
}

#pragma GCC diagnostic pop


// The handler for the "board" command
static void aocmd_board_main( int argc, char * argv[] ) {
  if( argc==1 ) {
    aocmd_board_show();
    return;
  }
  if( argc>=2 && aocmd_cint_isprefix("clk",argv[1])) {
    if( argc==2 ) {
      aocmd_board_clk_show();
      return;
    }
    int clk;
    bool ok = aocmd_cint_parse_dec(argv[2],&clk) ;
    if( !ok ) { Serial.printf("ERROR: 'cpu' expected frequency, not '%s'\n",argv[2]); return; }
    setCpuFrequencyMhz(clk); //  240, 160, 80
    if( argv[0][0]!='@' ) aocmd_board_clk_show();
    return;
  }
  if( argc==2 && aocmd_cint_isprefix("reboot",argv[1])) {
    ESP.restart();
  }
  if( argc==2 && aocmd_cint_isprefix("stackoverflow",argv[1])) {
    aocmd_board_stackoverflow();
  }
  if( argc==2 && aocmd_cint_isprefix("assert",argv[1])) {
    AORESULT_ASSERT( 0==1 );
  }
  Serial.printf("ERROR: 'board' has unknown argument ('%s')\n", argv[1] ); return;
}



// The long help text for the "board" command.
static const char aocmd_board_longhelp[] = 
  "SYNTAX: board\n"
  "- without arguments shows some board info (cpu, sensor, IRED, gauge)\n"
  "SYNTAX: board clk [<freq>]\n"
  "- without arguments shows cpu clock frequency\n"
  "- with argument sets cpu clock frequency\n"
  "- valid values are 10, 20, 40, 80, 160, 240\n"
  "SYNTAX: board reboot | stackoverflow | assert\n"
  "- resets the ESP (controlled or with a stack overflow, or an assert)\n"
  "- this does not reset other components (OSP nodes, OLED) on the board\n"
  "NOTES:\n"
  "- supports @-prefix to suppress output\n"
;


/*!
    @brief  Registers the built-in "board" command with the command interpreter.
    @return Number of remaining registration slots (or -1 if registration failed).
    @note   The aocmd_init calls this function, so normal client code does not need to call it.
    @note   If client code overrides the default registration by implementing 
            its own aocmd_register() then this function could be called from there.
*/
int aocmd_board_register() {
  return aocmd_cint_register(aocmd_board_main, "board", "board info and commands", aocmd_board_longhelp);
}

