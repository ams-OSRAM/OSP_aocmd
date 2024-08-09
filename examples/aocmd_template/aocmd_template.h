// aocmd_template.h - a template for an application with a command handler
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
#ifndef _AOCMD_TEMPLATE_H_
#define _AOCMD_TEMPLATE_H_


// Application version (and its history)
#define AOCMD_TEMPLATE_VERSION "2.2"
// 20240627  2.2  renamed to aocmd_template with underscore
// 20240609  2.1  renamed command 'tele' to 'osp'
// 20240522  2.0  renamed to application template
// 20240509  1.6  init for aospi and aoosp (for "osp")
// 20240508  1.5  Added banner
// 20240508  1.0  Created


// Application long name
#define AOCMD_TEMPLATE_LONGNAME "Application template"


// Application banner
#define AOCMD_TEMPLATE_BANNER "\n\n\n\n"\
  " _______                   _       _\n"\
  "|__   __|                 | |     | |\n"\
  "   | | ___ _ __ ___  _ __ | | __ _| |_ ___\n"\
  "   | |/ _ \\ '_ ` _ \\| '_ \\| |/ _` | __/ _ \\\n"\
  "   | |  __/ | | | | | |_) | | (_| | ||  __/\n"\
  "   |_|\\___|_| |_| |_| .__/|_|\\__,_|\\__\\___|\n"\
  "                    | |\n"\
  "                    |_|\n"\
  // https://patorjk.com/software/taag/#p=display&v=2&f=Big&t=Template

#endif
