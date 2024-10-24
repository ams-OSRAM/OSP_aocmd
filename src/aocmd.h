// aocmd.h - command interpreter (over UART/USB) and handlers for telegrams
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
#ifndef _AOCMD_H_
#define _AOCMD_H_


// Identifies lib version
#define AOCMD_VERSION "0.5.6"


// Include the (headers of the) modules of this app
#include <aocmd_cint.h>    // the command interpreter
#include <aocmd_echo.h>    // the command handler for "echo" 
#include <aocmd_help.h>    // the command handler for "help" 
#include <aocmd_version.h> // the command handler for "version"
#include <aocmd_board.h>   // the command handler for "board"
#include <aocmd_file.h>    // the command handler for "file" 
#include <aocmd_osp.h>     // the command handler for "osp"
#include <aocmd_said.h>    // the command handler for "said"


// Initializes the aocmd library (command interpreter, command registration, file system, telegram parser).
void aocmd_init(); 



// Instead of registering all commands contained in the library individually, use this convenience function to register all.
void aocmd_register();


#endif
