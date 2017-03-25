#include "power.h"
#include "humble_rtos.h"

#ifdef MCU_MSP430F5529_

#include <stdint.h>
#include <msp430f5529.h>

#define PMM_COREV_MASK				(0x03u)
#define POWER_VCORE_LVL_0 			(0x00u)
#define POWER_VCORE_LVL_1 			(0x01u)
#define POWER_VCORE_LVL_2           (0x02u)
#define POWER_VCORE_LVL_3           (0x03u)

#define POWER_GET_VCORE_LEVEL()		(vcore_level)

static uint8_t vcore_level = 0;

static void power_set_vcore_level_up(uint8_t level){

	/* Ensures level parameter is a number in range from 0 to 3 */
	level = level & 0x03;

	/* Open PMM registers for write access */
	PMMCTL0_H = PMMPW_H;

	/* Step 1: Program SVM_H and SVS_L to the next level to ensure DVcc
	   is high enough for the next Vcore level. Program the SVM_L to the next
	   level and wait for (SVSMLDLYIFG) to be set */
	SVSMHCTL = SVSHE + SVSHRVL0 * level + SVMHE + SVSMHRRL0 * level;
	SVSMLCTL = SVSLE + SVMLE + SVSMLRRL0 * level;
	while ((PMMIFG & SVSMLDLYIFG) == 0);
	PMMIFG &= ~(SVMLVLRIFG + SVMLIFG);

	/* Step 2: Program PMMCOREV to the next Vcore level */
	PMMCTL0_L = PMMCOREV0 * level;

	/* Step 3: Wait for the voltage level reached (SVMLVLRIFG) flag */
	if (PMMIFG & SVMLIFG){
	    while ((PMMIFG & SVMLVLRIFG) == 0);
	}

	/* Step 4: Program the SVS_L to the next level */
	SVSMLCTL = SVSLE + SVSLRVL0 * level + SVMLE + SVSMLRRL0 * level;

	/* Lock PMM registers for write access */
	PMMCTL0_H = 0x00;

	vcore_level++;
}

void power_init_vcore_highest_level(void){
	switch(POWER_GET_VCORE_LEVEL()){
		case POWER_VCORE_LVL_0:
			power_set_vcore_level_up(POWER_VCORE_LVL_1);
			power_set_vcore_level_up(POWER_VCORE_LVL_2);
			power_set_vcore_level_up(POWER_VCORE_LVL_3);
			break;
		case POWER_VCORE_LVL_1:
			power_set_vcore_level_up(POWER_VCORE_LVL_2);
			power_set_vcore_level_up(POWER_VCORE_LVL_3);
			break;
		case POWER_VCORE_LVL_2:
			power_set_vcore_level_up(POWER_VCORE_LVL_3);
			break;
		default:
			break;
	}
}

#endif /* MCU_MSP430F5529_ */
