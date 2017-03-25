#ifndef HUMBLE_RTOS_H_
#define HUMBLE_RTOS_H_

#define DEBUG 1
#define MCU_MSP430F5529_ 1

#include <stdint.h>


#define MAX_PRIORITY_LEVELS		(10u) // MUST BE LOWER THAN 17
#define MAX_THREADS				(10u)
#define STACK_SIZE				(100u)

typedef struct _task_t {
	uint16_t *sp;
	struct _task_t *prev, *next;
	uint8_t base_priority : 4;
	uint8_t real_priority : 4;
	uint8_t id;
	uint16_t sleep;
}task_t;

typedef struct _semaphore_t {
	task_t *block_list;
	int8_t value;
	uint8_t owner_id;
}semaphore_t;

typedef struct {
	task_t *head;
}task_head_t;

extern void enable_interrupts(void);
extern void disable_interrupts(void);
extern void humble_rtos_scheduler_launch(void);
extern void quantum_isr(void);

void humble_rtos_init(void);
void humble_rtos_scheduler_init(uint16_t time_slice);
void humble_rtos_schedule_next(void);
void humble_rtos_create_thread(void (*thread)(void), uint8_t priority);
void humble_rtos_msevent_timer_init(void);
void humble_rtos_msleep(uint16_t time_ms);

void humble_rtos_sem_init(semaphore_t *sem, int8_t value);
void humble_rtos_sem_wait(semaphore_t *sem);
void humble_rtos_sem_post(semaphore_t *sem);


#endif /* HUMBLE_RTOS_H_ */
