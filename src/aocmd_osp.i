// aocmd_osp.i - command handler for the "osp" command - to send and receive OSP telegrams
/*****************************************************************************
 * Copyright 2024,2025 by ams OSRAM AG                                       *
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


// This file is intented to be read into an array.
// It gives info on all telegram variants.


//    tid, "name"          ,ser,sizes,res,   "teleargs"                                , "respargs"                     , "description.")
ITEM(0x00, "reset"         , 0 ,0x001, 0 ,   0                                         , 0                              , "Performs a reset of all OSP nodes (includes com mode, excludes OTP-P2RAM). Typically broadcast (addr=000).")
ITEM(0x01, "clrerror"      , 0 ,0x001, 0 ,   0                                         , 0                              , "Clears all error flags in a node.")
ITEM(0x02, "initbidir"     , 1 ,0x001, 2 ,   0                                         , "temp stat"                    , "Sets direction to BiDir and initiates addressing. Last node returns its address. Typically serial cast (addr=001).")
ITEM(0x03, "initloop"      , 1 ,0x001, 2 ,   0                                         , "temp stat"                    , "Sets direction to Loop and initiates addressing. Last node returns its address. Typically serial cast (addr=001).")
ITEM(0x04, "gosleep"       , 0 ,0x001, 0 ,   0                                         , 0                              , "Sets a node into SLEEP state.")
ITEM(0x05, "goactive"      , 0 ,0x001, 0 ,   0                                         , 0                              , "Sets a node into ACTIVE state.")
ITEM(0x06, "godeepsleep"   , 0 ,0x001, 0 ,   0                                         , 0                              , "Sets a node into DEEPSLEEP state.")
ITEM(0x07, "identify"      , 0 ,0x001, 4 ,   0                                         , "id0 id1 id2 id3"              , "Returns the 32-bit node identification code (4 bits reserved, 10 bits manufacturer, 12 bits part, 6 bits revision).")
ITEM(0x08, "p4errbidir"    , 1 ,0x001, 2 ,   0                                         , "temp stat"                    , "Ping for error in direction BiDir (first node with error answers, otherwise forwards).")
ITEM(0x09, "p4errloop"     , 1 ,0x001, 2 ,   0                                         , "temp stat"                    , "Ping for error in direction Loop (first node with error answers, otherwise forwards).")
ITEM(0x0A, "asktinfo"      , 1 ,0x005, 2 ,   "max min"                                 , "max min"                      , "Returns the aggregated max and min temperature across the OSP chain.")
ITEM(0x0B, "askvinfo"      , 1 ,0x005, 2 ,   "max min"                                 , "max min"                      , "Returns the aggregated max and min voltage headroom among all drivers across the OSP chain.")
ITEM(0x0C, "readmult"      , 0 ,0x001, 2 ,   0                                         , "groups1 groups0"              , "Bits 0 to E in register MULT assigns a node to a group (3F0..3FE). This commands returns MULT.")
ITEM(0x0D, "setmult"       , 0 ,0x004, 0 ,   "groups1 groups0"                         , 0                              , "Configures MULT, a 15-bit mask indicating to which groups the node belongs.")
ITEM(0x0E, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x0F, "sync"          , 0 ,0x001, 0 ,   0                                         , 0                              , "A sync event (via external pin or via this command), activates all drivers with pre-configured settings.")
ITEM(0x10, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0 )
ITEM(0x11, "idle"          , 0 ,0x001, 0 ,   0                                         , 0                              , "Set P2RAM in idle mode.")
ITEM(0x12, "foundry"       , 0 ,0x001, 0 ,   0                                         , 0                              , "Set P2RAM in foundry mode.")
ITEM(0x13, "cust"          , 0 ,0x001, 0 ,   0                                         , 0                              , "Set P2RAM in customer mode.")
ITEM(0x14, "burn"          , 0 ,0x001, 0 ,   0                                         , 0                              , "Set P2RAM to burning.")
ITEM(0x15, "aread"         , 0 ,0x001, 0 ,   0                                         , 0                              , "Set P2RAM analog read.")
ITEM(0x16, "load"          , 0 ,0x001, 0 ,   0                                         , 0                              , "P2RAM load.")
ITEM(0x17, "gload"         , 0 ,0x001, 0 ,   0                                         , 0                              , "P2RAM gload.")
ITEM(0x18, "i2cread"       , 0 ,0x008, 0 ,   "daddr raddr count"                       , 0                              , "Initiates a read over the I2C bus. Read bytes are temporarily stored; get them with 1E/readlast telegram.")
ITEM(0x19, "i2cwrite"      , 0 ,0x1F8, 0 ,   "daddr raddr byte..."                     , 0                              , "Writes the bytes from the telegram payload over I2C bus.")
ITEM(0x1A, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x1B, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x1C, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x1D, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x1E, "readlast"      , 0 ,0x001, 8 ,   0                                         , "byte..."                      , "Returns bytes from last I2C read telegram (see 18/i2cread).")
ITEM(0x1F, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x20, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x21, "clrerror_sr"   , 0 ,0x001, 2 ,   0                                         , "temp stat"                    , "Clears all error flags in a node. Returns TEMP&STAT.")
ITEM(0x22, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x23, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x24, "gosleep_sr"    , 0 ,0x001, 2 ,   0                                         , "temp stat"                    , "Sets a node into SLEEP state. Returns TEMP&STAT.")
ITEM(0x25, "goactive_sr"   , 0 ,0x001, 2 ,   0                                         , "temp stat"                    , "Sets a node into ACTIVE state. Returns TEMP&STAT.")
ITEM(0x26, "godeepsleep_sr", 0 ,0x001, 2 ,   0                                         , "temp stat"                    , "Sets a node into DEEPSLEEP state. Returns TEMP&STAT.")
ITEM(0x27, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x28, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x29, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x2A, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x2B, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x2C, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x2D, "setmult_sr"    , 0 ,0x004, 2 ,   "groups1 groups0"                         , "temp stat"                    , "Configures MULT. Returns TEMP&STAT.")
ITEM(0x2E, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x2F, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x30, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x31, "idle_sr"       , 0 ,0x001, 2 ,   0                                         , "temp stat"                    , "Set P2RAM in idle mode. Returns TEMP&STAT.")
ITEM(0x32, "foundry_sr"    , 0 ,0x001, 2 ,   0                                         , "temp stat"                    , "Set P2RAM in foundry mode. Returns TEMP&STAT.")
ITEM(0x33, "cust_sr"       , 0 ,0x001, 2 ,   0                                         , "temp stat"                    , "Set P2RAM in customer mode. Returns TEMP&STAT.")
ITEM(0x34, "burn_sr"       , 0 ,0x001, 2 ,   0                                         , "temp stat"                    , "Set P2RAM to burning. Returns TEMP&STAT.")
ITEM(0x35, "aread_sr"      , 0 ,0x001, 2 ,   0                                         , "temp stat"                    , "Set P2RAM analog read. Returns TEMP&STAT.")
ITEM(0x36, "load_sr"       , 0 ,0x001, 2 ,   0                                         , "temp stat"                    , "Set P2RAM load. Returns TEMP&STAT.")
ITEM(0x37, "gload_sr"      , 0 ,0x001, 2 ,   0                                         , "temp stat"                    , "Set P2RAM gload. Returns TEMP&STAT.")
ITEM(0x38, "i2cread_sr"    , 0 ,0x008, 2 ,   "daddr raddr count"                       , "temp stat"                    , "Initiated a read over the I2C bus. Returns TEMP&STAT.")
ITEM(0x39, "i2cwrite_sr"   , 0 ,0x1F8, 2 ,   "daddr raddr byte..."                     , "temp stat"                    , "Writes the bytes from the telegram payload over I2C bus. Returns TEMP&STAT.")
ITEM(0x3A, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x3B, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x3C, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x3D, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x3E, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x3F, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x40, "readstat"      , 0 ,0x001, 1 ,   0                                         , "stat"                         , "Returns the system STAT register: SAID:STATE|TSTOTP|OV|CE|LOS|OT|UV; RGBI:STATE|OTP|COM|CE|LOS|OT|UV; STATE:00=uninit,01=sleep,10=active,11=deepsleep.")
ITEM(0x41, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x42, "readtempstat"  , 0 ,0x001, 2 ,   0                                         , "temp stat"                    , "Returns the TEMPerature and the system STAT.")
ITEM(0x43, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x44, "readcomst"     , 0 ,0x001, 1 ,   0                                         , "comst"                        , "Returns the communication mode of both SIO ports: SAID:DIR|SIO2|SIO1; RGBI:SIO2|SIO1; SIOx:00=LVDS,01=EOL,10=MCU,11=CAN; DIR:0=BIDIR,1=LOOP.")
ITEM(0x45, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x46, "readledst"     , 0 ,0x001, 1 ,   0                                         , "ledst"                        , "Returns LED status (no channel) RSV|RO|GO|BO|RSV|RS|GS|BS (R for red, G for green, B for blue, O for open, S for short, RSV reserved bit.")
ITEM(0x46, "readledstchn"  , 0 ,0x002, 1 ,   "chn"                                     , "ledst"                        , "Returns LED status for channel RSV|RO|GO|BO|RSV|RS|GS|BS (R for red, G for green, B for blue, O for open, S for short, RSV reserved bit.")
ITEM(0x47, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x48, "readtemp"      , 0 ,0x001, 1 ,   0                                         , "temp"                         , "Returns the TEMPerature (Celsius: for RGBI 1.08xTEMP–126, for SAID TEMP–86).")
ITEM(0x49, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x4A, "readotth"      , 0 ,0x001, 3 ,   0                                         , "cycle low high"               , "Returns Over Temperature ThresHold configuration.")
ITEM(0x4B, "setotth"       , 0 ,0x008, 0 ,   "cycle th1 th0"                           , 0                              , "Configures the Over Temperature ThresHold (cycle length, low and high value).")
ITEM(0x4C, "readsetup"     , 0 ,0x001, 1 ,   0                                         , "flags"                        , "Returns node SETUP.")
ITEM(0x4D, "setsetup"      , 0 ,0x002, 0 ,   "flags"                                   , 0                              , "Configures SETUP (PWM, SPI clock invert, TEMPerature update rate, error behavior).")
ITEM(0x4E, "readpwm"       , 0 ,0x001, 6 ,   0                                         , "red1 red0 grn1 grn0 blu1 blu0", "Returns current PWM setting (for all three LEDs).")
ITEM(0x4E, "readpwmchn"    , 0 ,0x002, 6 ,   "chn"                                     , "red1 red0 grn1 grn0 blu1 blu0", "Returns current PWM setting (for all three LEDs) of the requested channel.")
ITEM(0x4F, "setpwm"        , 0 ,0x040, 0 ,   "red1 red0 grn1 grn0 blu1 blu0"           , 0                              , "Sets PWM for RGB 3x(1+15) bits (MSB is night/day time).")
ITEM(0x4F, "setpwmchn"     , 0 ,0x100, 0 ,   "chn unused red1 red0 grn1 grn0 blu1 blu0", 0                              , "Sets PWM for RGB: 1st byte is the channel, 2nd is padding, then 3x(15+1) bits for PWM (LSB is dithering).")
ITEM(0x50, "readcurchn"    , 0 ,0x002, 2 ,   "chn"                                     , "flag-cred cgrn-cblu"          , "Returns the current level of the requested channel.")
ITEM(0x51, "setcurchn"     , 0 ,0x008, 0 ,   "chn flag-cred cgrn-cblu"                 , 0                              , "Set the current level of a specified channel (reserved/sync/hybrid/dither flags, 3x4bit current).")
ITEM(0x52, "readtcoeff"    , 0 ,0x002, 4 ,   "chn"                                     , "tref tred tgrn tblu"          , "Returns the temperature coefficients of the requested channel.")
ITEM(0x53, "settcoeff"     , 0 ,0x020, 0 ,   "chn tref tred tgrn tblu"                 , 0                              , "Sets RGB temperature coefficients (and a reference) of a specified channel.")
ITEM(0x54, "readadc"       , 0 ,0x001, 1 ,   0                                         , "aaddr"                        , "Returns the ADC configuration (the data is in reg 5C/readadcdat).")
ITEM(0x55, "setadc"        , 0 ,0x002, 0 ,   "aaddr"                                   , 0                              , "Configures the ADC to measure Vf of the 3x3 LED.")
ITEM(0x56, "readi2ccfg"    , 0 ,0x001, 1 ,   0                                         , "flasg"                        , "Returns I2C status/conf register (INT, 12bit-mode, last-ack, I2C-busy, I2C-speed).")
ITEM(0x57, "seti2ccfg"     , 0 ,0x002, 0 ,   "flags"                                   , 0                              , "Configures the I2C master.")
ITEM(0x58, "readotp"       , 0 ,0x002, 8 ,   "oaddr"                                   , "byte..."                      , "Returns 8 bytes from OTP starting at the ADDR passed as argument.")
ITEM(0x59, "setotp"        , 0 ,0x1FC, 0 ,   "byte... oaddr"                           , 0                              , "Writes 8 bytes to specified OTP address (always 8).")
ITEM(0x5A, "readtestdata"  , 0 ,0x002, 2 ,   "taddr"                                   , "byte byte"                    , "Reads test data.")
ITEM(0x5B, "settestdata"   , 0 ,0x004, 0 ,   "byte byte"                               , 0                              , "Writes test data.")
ITEM(0x5C, "readadcdat"    , 0 ,0x001, 1 ,   0                                         , "saddr"                        , "Returns ADC value configured via 55/setadc.")
ITEM(0x5D, "testscan"      , 0 ,0x002, 0 ,   "saddr"                                   , 0                              , "Executes test scan.")
ITEM(0x5E, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x5F, "settestpw"     , 0 ,0x040, 0 ,   "PW5 PW4 PW3 PW2 PW1 PW0"                 , 0                              , "Authenticates user, for OTP or test mode.")
ITEM(0x60, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x61, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x62, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x63, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x64, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x65, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x66, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x67, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x68, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x69, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x6A, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x6B, "setotth_sr"    , 0 ,0x008, 2 ,   "cycle low high"                          , "temp stat"                    , "Configures the Over Temperature ThresHold. Returns TEMP&STAT.")
ITEM(0x6C, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x6D, "setsetup_sr"   , 0 ,0x002, 2 ,   "flags"                                   , "temp stat"                    , "Configures SETUP. Returns TEMP&STAT.")
ITEM(0x6E, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x6F, "setpwm_sr"     , 0 ,0x040, 2 ,   "red1 red0 grn1 grn0 blu1 blu0"           , "temp stat"                    , "Sets PWM for RGB. Returns TEMP&STAT.")
ITEM(0x6F, "setpwmchn_sr"  , 0 ,0x100, 2 ,   "chn unused red1 red0 grn1 grn0 blu1 blu0", "temp stat"                    , "Sets PWM for RGB for a channel. Returns TEMP&STAT.")
ITEM(0x70, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)                                     
ITEM(0x71, "setcurchn_sr"  , 0 ,0x008, 2 ,   "chn flag-cred cgrn-cblu"                 , "temp stat"                    , "Set the current level of a specified channel (reserved/sync/hybrid/dither flags, 3x4 current). Returns TEMP&STAT.")
ITEM(0x72, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x73, "settcoeff_sr"  , 0 ,0x020, 2 ,   "chn tref tred tgrn tblu"                 , "temp stat"                    , "Sets RGB temperature coefficients. Returns TEMP&STAT.")
ITEM(0x74, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x75, "setadc_sr"     , 0 ,0x002, 2 ,   "aaddr"                                   , "temp stat"                    , "Configures the ADC to measure Vf. Returns TEMP&STAT.")
ITEM(0x76, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)                                     
ITEM(0x77, "seti2ccfg_sr"  , 0 ,0x002, 2 ,   "flags"                                   , "temp stat"                    , "Configures the I2C master. Returns TEMP&STAT.")
ITEM(0x78, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)                                     
ITEM(0x79, "setotp_sr"     , 0 ,0x100, 2 ,   "byte... oaddr"                           , "temp stat"                    , "Writes 8 bytes to specified OTP address. Returns TEMP&STAT.")
ITEM(0x7A, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x7B, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x7C, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x7D, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x7E, 0               , 0 ,0x000, 0 ,   0                                         , 0                              , 0)
ITEM(0x7F, "settestpw_sr"  , 0 ,0x040, 2 ,   "PW5 PW4 PW3 PW2 PW1 PW0"                 , "temp stat"                    , "Authenticates user, for OTP or test mode. Returns TEMP&STAT.")
