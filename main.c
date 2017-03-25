#include <msp430f5529.h>
#include "humble_rtos.h"
#include "clock.h"
#include "watchdog.h"
#include "ports.h"

void func_teste1(void);
void func_teste2(void);
void func_teste3(void);

semaphore_t sem;

int main(void) {
	humble_rtos_init();
	humble_rtos_sem_init(&sem,1);
	humble_rtos_create_thread(func_teste1,2);
	humble_rtos_create_thread(func_teste2,3);
	humble_rtos_create_thread(func_teste3,4);
	humble_rtos_msevent_timer_init();
	humble_rtos_scheduler_init(TIMESLICE_1_MILISECOND);
}

void func_teste1(void){
	int32_t count;
	P1DIR |= BIT2;
	P1OUT &= ~BIT2;

	while(1){
		humble_rtos_msleep(2);
		humble_rtos_sem_wait(&sem);
		for (count = 0; count < 2000; ++count) {
			P1OUT ^= BIT2;
		}
		humble_rtos_sem_post(&sem);
	}
}

void func_teste2(void){
	int32_t count;
	P1DIR |= BIT3;
	P1OUT &= ~BIT3;

	while(1){
		for (count = 0; count < 2000; ++count) {
			P1OUT ^= BIT3;
		}
		humble_rtos_msleep(1);
	}
}


void func_teste3(void){
	int32_t count;
	P1DIR |= BIT4;
	P1OUT &= ~BIT4;
	while(1){
		humble_rtos_sem_wait(&sem);
		for (count = 0; count < 5000; ++count) {
			P1OUT ^= BIT4;
		}
		humble_rtos_sem_post(&sem);
	}
}
