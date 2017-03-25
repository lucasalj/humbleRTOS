#include "clock.h"
#include "humble_rtos.h"

#ifdef MCU_MSP430F5529_

#include <msp430f5529.h>
#include "power.h"

void clock_init_maximum_speed(void){

	power_init_vcore_highest_level();

	UCSCTL3 = SELREF_2;			// Set DCO FLL reference = REFO
	UCSCTL4 |= SELA_2;          // Set ACLK = REFO
	__bis_SR_register(SCG0);    // Disable the FLL control loop
	UCSCTL0 = 0x0000;           // Set lowest possible DCOx, MODx
	UCSCTL1 = DCORSEL_7;        // Select DCO range 50MHz operation
	UCSCTL2 = FLLD_0 + 762;     // Set DCO Multiplier for 25MHz
	                            // (N + 1) * FLLRef = Fdco
	                            // (762 + 1) * 32768 = 25MHz
	                            // Set FLL Div = fDCOCLK/2
	__bic_SR_register(SCG0);    // Enable the FLL control loop

	// Worst-case settling time for the DCO when the DCO range bits have been
	// changed is n x 32 x 32 x f_MCLK / f_FLL_reference. See UCS chapter in 5xx
	// UG for optimization.
	// 32 x 32 x 25 MHz / 32,768 Hz ~ 780k MCLK cycles for DCO to settle
	__delay_cycles(782000);

	// Loop until XT1,XT2 & DCO stabilizes - In this case only DCO has to stabilize
	do
	{
		UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + DCOFFG);
	                                            // Clear XT2,XT1,DCO fault flags
		SFRIFG1 &= ~OFIFG;                      // Clear fault flags
	}while (SFRIFG1&OFIFG);                     // Test oscillator fault flag

}

#endif /* MCU_MSP430F5529_ */

