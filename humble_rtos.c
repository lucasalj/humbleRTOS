#include "humble_rtos.h"
#include "ports.h"
#include "watchdog.h"
#include "msp430f5529.h"
#include "clock.h"


#define HIGHER_PRIORITY_THAN(task_a, task_b) \
			task_a->real_priority < task_b->real_priority

uint8_t RTOS_RUNNING = 0;

uint16_t stacks[MAX_THREADS][STACK_SIZE];
task_t threads[MAX_THREADS];

task_head_t ready_tasks[MAX_PRIORITY_LEVELS] = {{0},};

task_t *sleeping_task_queue = 0;

task_t *run_task;

static void humble_rtos_ready_task_insert_by_id(uint8_t task_id,
												 uint8_t priority);
static task_t* humble_rtos_task_remove_by_id(task_t **head,
															int8_t id);

void humble_rtos_init(void){
    WATCHDOG_OFF();
	disable_interrupts();
    ports_set_output_low();
	clock_init_maximum_speed();
}

void humble_rtos_scheduler_init(uint16_t time_slice){
	TA0EX0 = 0;
	TA0CCR0 = time_slice-1;
	TA0CTL = TACLR;
	TA0CTL |= TASSEL__SMCLK | MC__UP | TAIE;
	RTOS_RUNNING = 1;
	humble_rtos_scheduler_launch();
}

void humble_rtos_msevent_timer_init(void){
	TA1EX0 = 0;
	TA1CCR0 = TIMESLICE_1_MILISECOND-1;
	TA1CTL = TACLR;
	TA1CTL |= TASSEL__SMCLK | MC__UP | TAIE;
}

static inline void humble_rtos_task_init(uint8_t task_id, uint8_t priority,
							      void (*thread)(void)){
	priority &= 0xf;
	uint32_t thread_addr = (uint32_t)thread;
	humble_rtos_ready_task_insert_by_id(task_id,priority);
	threads[task_id].id = task_id;
	threads[task_id].sleep = 0;
	threads[task_id].base_priority = priority;
	threads[task_id].real_priority = priority;
	threads[task_id].sp = &stacks[task_id][STACK_SIZE-26];
	stacks[task_id][STACK_SIZE-1] = (thread_addr >> 16) & 0x000F; // PC(19:16)
																  // bits(3:0)
	stacks[task_id][STACK_SIZE-2] = thread_addr & 0xFFFF; // PC(15:0)
	stacks[task_id][STACK_SIZE-3] = 0x0005;
	stacks[task_id][STACK_SIZE-4] = 0x1515;
	stacks[task_id][STACK_SIZE-5] = 0x0004;
	stacks[task_id][STACK_SIZE-6] = 0x1414;
	stacks[task_id][STACK_SIZE-7] = 0x0003;
	stacks[task_id][STACK_SIZE-8] = 0x1313;
	stacks[task_id][STACK_SIZE-9] = 0x0002;
	stacks[task_id][STACK_SIZE-10] = 0x1212;
	stacks[task_id][STACK_SIZE-11] = 0x0001;
	stacks[task_id][STACK_SIZE-12] = 0x1111;
	stacks[task_id][STACK_SIZE-13] = 0x0000;
	stacks[task_id][STACK_SIZE-14] = 0x1010;
	stacks[task_id][STACK_SIZE-15] = 0x0009;
	stacks[task_id][STACK_SIZE-16] = 0x0909;
	stacks[task_id][STACK_SIZE-17] = 0x0008;
	stacks[task_id][STACK_SIZE-18] = 0x0808;
	stacks[task_id][STACK_SIZE-19] = 0x0007;
	stacks[task_id][STACK_SIZE-20] = 0x0707;
	stacks[task_id][STACK_SIZE-21] = 0x0006;
	stacks[task_id][STACK_SIZE-22] = 0x0606;
	stacks[task_id][STACK_SIZE-23] = 0x0005;
	stacks[task_id][STACK_SIZE-24] = 0x0505;
	stacks[task_id][STACK_SIZE-25] = 0x0004;
	stacks[task_id][STACK_SIZE-26] = 0x0404;
}


void humble_rtos_create_thread(void (*thread)(void), uint8_t priority){
	static uint32_t task_counter=0;
	disable_interrupts();
	priority &= 0xf;
	humble_rtos_task_init(task_counter, priority, thread);
	if(!RTOS_RUNNING && (task_counter == 0 ||
			priority < run_task->base_priority)){
		run_task = &threads[task_counter];
	}
	task_counter++;
	enable_interrupts();
}

static void humble_rtos_ready_task_insert_by_id(uint8_t task_id,
												 uint8_t priority){
	priority &= 0xf;
	if(ready_tasks[priority].head == 0){
		ready_tasks[priority].head = &threads[task_id];
		threads[task_id].next = threads[task_id].prev = &threads[task_id];
	}
	else{
		task_t *first = ready_tasks[priority].head;
		task_t *last = first->prev;
		last->next = &threads[task_id];
		first->prev = &threads[task_id];
		threads[task_id].prev = last;
		threads[task_id].next = first;
	}
}

void humble_rtos_schedule_next(void){
	uint8_t i;
	ready_tasks[run_task->real_priority].head =
			ready_tasks[run_task->real_priority].head != 0 ?
			ready_tasks[run_task->real_priority].head->next : 0;
	for (i = 0; i < MAX_PRIORITY_LEVELS; ++i) {
		if(ready_tasks[i].head != 0){
			run_task = ready_tasks[i].head;
			break;
		}
	}
}

#pragma vector=TIMER0_A1_VECTOR
__interrupt void timer0_a1_isr(void){
	quantum_isr();
}

#pragma vector=TIMER1_A1_VECTOR
__interrupt void timer1_a1_isr(void){
	volatile uint16_t flags = TA1IV;
	task_t *iterator = sleeping_task_queue;
	int task_removed;
	if(iterator != 0){
		do{
			task_removed = 0;
			iterator->sleep--;
			if(iterator->sleep == 0){
				task_t *wakeup_task = iterator;
				iterator = iterator->next;
				humble_rtos_task_remove_by_id(&sleeping_task_queue,
						wakeup_task->id);
				humble_rtos_ready_task_insert_by_id(wakeup_task->id,
						wakeup_task->real_priority);
				task_removed = 1;
			}
			else{
				iterator = iterator->next;
			}
		}while((sleeping_task_queue != 0) && (task_removed || (iterator != sleeping_task_queue)));
	}
}

static inline void humble_rtos_suspend(void){
	TA0CTL = TACLR;
	TA0CTL |= TASSEL__SMCLK | MC__UP | TAIE;
	TA0CTL |= TAIFG;
}

static task_t* humble_rtos_task_remove_first(task_t **head){
	task_t *removed_task;
	if(*head == 0){
		removed_task = 0;
	}
	else if((*head)->next == *head){
		removed_task = *head;
		*head = 0;
	}
	else{
		removed_task = *head;
		*head = removed_task->next;
		removed_task->next->prev = removed_task->prev;
		removed_task->prev->next = removed_task->next;
	}
	return removed_task;
}

static task_t* humble_rtos_task_remove_by_id(task_t **head,
															int8_t id){
	task_t *removed_task = 0;
	if(*head != 0){
		if((*head)->next == (*head) && (*head)->id == id){
			removed_task = *head;
			*head = 0;
		}
		else {
			task_t *iterator = *head;
			if(iterator->id != id){
				do{
					iterator = iterator->next;
				}while( iterator != (*head) && iterator->id != id);
			}
			removed_task = iterator;
			removed_task->next->prev = removed_task->prev;
			removed_task->prev->next = removed_task->next;
			if(removed_task->id == (*head)->id){
				*head = removed_task->next;
			}
		}
	}
	return removed_task;
}

static void humble_rtos_task_insert_by_priority(task_t **head,
		task_t* insert_task){
	if(*head == 0){
		insert_task->next = insert_task;
		insert_task->prev = insert_task;
		*head = insert_task;
	}
	else if((*head)->next == (*head)){
		insert_task->next = *head;
		insert_task->prev = *head;
		(*head)->prev = insert_task;
		(*head)->next = insert_task;
		*head = (*head)->real_priority > insert_task->real_priority ?
				insert_task : *head;
	}
	else{
		task_t *iterator = *head;
		if(iterator->real_priority > insert_task->real_priority){
			insert_task->prev = iterator->prev;
			insert_task->next = iterator;
			insert_task->next->prev = insert_task;
			insert_task->prev->next = insert_task;
			*head = insert_task;
		}
		else {
			do{
				iterator = iterator->next;
			}while(iterator != *head &&
					iterator->real_priority < insert_task->real_priority);
			if(iterator == *head){
				insert_task->prev = iterator->prev;
				insert_task->next = iterator;
				iterator->prev->next = insert_task;
				iterator->prev = insert_task;
			}
			else{
				insert_task->prev = iterator->prev;
				insert_task->next = iterator;
				iterator->prev->next = insert_task;
				iterator->prev = insert_task;
			}
		}
	}
}

void humble_rtos_sem_init(semaphore_t *sem, int8_t value){
	sem->block_list = 0;
	sem->value = value;
	sem->owner_id = 0;
}

/**
 * @brief This function implements a blocking wait on a semaphore. It has an
 * 	      inheritance priority mechanism, which assumes that the last thread
 * 	      that has executed a wait on the semaphore is the owner, and so if
 * 	      its priority is lower (higher value) than the thread that is going
 * 	      to be blocked on the same semaphore, the semaphore owner stops
 * 	      running on lower priority and inherit the value of the highest
 * 	      priority blocked thread.
 * @param semaphore_t *sem: The address of the semaphore to wait on
 * @return void
 */
void humble_rtos_sem_wait(semaphore_t *sem){
	disable_interrupts();
	sem->value--;
	if(sem->value < 0){
		if( HIGHER_PRIORITY_THAN(run_task,(&threads[sem->owner_id])) ){
			humble_rtos_task_remove_by_id(
					&(ready_tasks[threads[sem->owner_id].real_priority].head),
					sem->owner_id);
			threads[sem->owner_id].real_priority = run_task->real_priority;
			humble_rtos_ready_task_insert_by_id(sem->owner_id,
					threads[sem->owner_id].real_priority);
		}
		humble_rtos_task_remove_by_id(
				&(ready_tasks[run_task->real_priority].head), run_task->id);
		humble_rtos_task_insert_by_priority(&sem->block_list,run_task);
		enable_interrupts();
		humble_rtos_suspend();
	}
	sem->owner_id = run_task->id;
	enable_interrupts();
}

/**
 * @brief This function implements a post on a semaphore. It unblocks the
 *        highest priority thread waiting on the semaphore, and if the
 *        semaphore owner is running on a elevated priority mode, reset its
 *        priority to original level.
 * @param semaphore_t *sem: The address of the semaphore to wait on
 * @return void
 */
void humble_rtos_sem_post(semaphore_t *sem){
	disable_interrupts();
	sem->value++;
	if(sem->value <= 0){
		task_t *free_thread = humble_rtos_task_remove_first(&sem->block_list);
		humble_rtos_ready_task_insert_by_id(free_thread->id,
				free_thread->real_priority);
		if(threads[sem->owner_id].base_priority !=
				threads[sem->owner_id].real_priority){
			humble_rtos_task_remove_by_id(
					&ready_tasks[threads[sem->owner_id].real_priority].head,
					sem->owner_id);
			threads[sem->owner_id].real_priority =
					threads[sem->owner_id].base_priority;
			humble_rtos_ready_task_insert_by_id(sem->owner_id,
					threads[sem->owner_id].real_priority);
		}
	}
	enable_interrupts();
}

static void humble_rtos_sleeping_task_insert(task_t *task){
	if(sleeping_task_queue == 0){
		sleeping_task_queue = task;
		task->next = task->prev = task;
	}
	else{
		task_t *iterator = sleeping_task_queue;
		while(iterator->next != sleeping_task_queue){
			iterator = iterator->next;
		}
		task->next = iterator->next;
		task->prev = iterator;
		iterator->next->prev = task;
		iterator->next = task;
	}
}

void humble_rtos_msleep(uint16_t time_ms){
	disable_interrupts();
	run_task->sleep = time_ms;
	humble_rtos_task_remove_by_id(&ready_tasks[run_task->real_priority].head,
			run_task->id);
	humble_rtos_sleeping_task_insert(run_task);
	enable_interrupts();
	humble_rtos_suspend();
}
