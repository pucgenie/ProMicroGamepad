/*@create-file:build.opt@
// An embedded build.opt file using a "C" block comment. The starting signature
// must be on a line by itself. The closing block comment pattern should be on a
// line by itself. Each line within the block comment will be space trimmed and
// written to build.opt, skipping blank lines and lines starting with '//', '*'
// or '#'.

 # is ignored because board library gets compiled without our headers
#-DUSB_CONFIG_POWER=24
 # USBCore_DISABLE_LEDS is a custom feature added to my board definition / libc source
-DUSBCore_DISABLE_LEDS=1
*/

#ifndef JOYSTICKBUTTON_INO_GLOBALS_H
#define JOYSTICKBUTTON_INO_GLOBALS_H

#endif
