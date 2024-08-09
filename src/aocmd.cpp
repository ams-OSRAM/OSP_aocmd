// aocmd.cpp - command interpreter (over UART/USB) and handlers for telegrams
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


#include <Arduino.h>    // Serial.printf
#include <aocmd.h>      // own


/*!
    @brief  Registers all commands contained in library aocmd with the command interpreter.
    @note   These commands are registered: echo, help, version, board, file, tele, said.
    @note   If client code wants a subset of the commands it should call the individual 
            aocmd_xxx_register() functions.
    @note   Order of registration is not relevant (command interpreter keeps them alphabetically). 
*/
void aocmd_register() {
  aocmd_echo_register(); 
  aocmd_help_register(); 
  aocmd_version_register(); 
  aocmd_board_register(); 
  aocmd_file_register(); 
  aocmd_osp_register(); 
  aocmd_said_register(); 
}


/*!
    @brief  Initializes the aocmd library 
            (command interpreter, file system, telegram parser).
    @note   In setup(), make sure Serial.begin() is called before aocmd_init().
            Library aocmd reads/writes chars from/to Serial.
    @note   In setup(), register commands, eg by calling aocmd_register().
    @note   In setup(), print the initial prompt with aocmd_cint_prompt().
    @note   In loop(), process incoming commands with aocmd_cint_pollserial().
    @note   If own commands also need to be registered in addition to the standard ones
            from this library, implement aocmd_register_extra()
            and let it call one or more xxx_cccc_register().
    @note   If only own commands need to be registered, implement aocmd_register()
            and let it call one or more xxx_cccc_register().
*/
void aocmd_init() {
  aocmd_cint_init();
  aocmd_file_init(); // The "file" command also contains the file system implementation.
  aocmd_osp_init(); // The "osp" command also contains the tx/rx parser implementation.
  Serial.printf("cmd: init\n");
}
