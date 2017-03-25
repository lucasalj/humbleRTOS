#include "ports.h"
#include "msp430f5529.h"

void ports_set_output_low(void){
	// General porpuse I/O function
	PASEL = 0x0000;
	PBSEL = 0x0000;
	PCSEL = 0x0000;
	PDSEL = 0x0000;

	// Ouput direction
	PADIR = 0xFFFF;
	PBDIR = 0xFFFF;
	PCDIR = 0xFFFF;
	PDDIR = 0xFFFF;

	// Output low
	PAOUT = 0x0000;
	PBOUT = 0x0000;
	PCOUT = 0x0000;
	PDOUT = 0x0000;
}
