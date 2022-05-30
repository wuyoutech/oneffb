#include <ioCC2544.h>
#include <ioCC254x_bitdef.h>

void system_init(void){
    // Set system clock source to HS XOSC, with no pre-scaling.
    CLKCONCMD = (CLKCONCMD & ~(CLKCON_OSC | CLKCON_CLKSPD)) | CLKCON_CLKSPD_32M;
    // Wait until clock source has changed.
    while (CLKCONSTA & CLKCON_OSC);
  
    /* Note the 32 kHz RCOSC starts calibrating, if not disabled. */
}