#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "queue.h"
#include "sem.h"
#include "thread.h"

struct semaphore {
	/* TODO Phase 1 */
	int internal_count;
	struct queue* waiting;
};

sem_t sem_create(size_t count)
{
	/* TODO Phase 1 */
	struct semaphore* sem = malloc(sizeof(struct semaphore));
	
	if(sem == NULL)
	{
		return NULL;
	}
	
	sem->internal_count = count;
	sem->waiting = queue_create(); 
	return sem;
}

int sem_destroy(sem_t sem)
{
	/* TODO Phase 1 */
	
	if(sem == NULL || queue_length(sem->waiting) != 0)
	{
		return -1;				//return -1 if sem is null or threads are
	}						//still being blocked.
	
	free(sem);
	queue_destroy(sem->waiting);
	
	return 0;
}

int sem_down(sem_t sem)
{
	/* TODO Phase 1 */

	if(sem == NULL)
	{	
		return -1;
	}
	
	enter_critical_section();

	while (sem->internal_count == 0){
		if(queue_enqueue(sem->waiting, (void *)pthread_self()) == 0)
		{
			thread_block();
		}
	}
	
	sem->internal_count--;

	exit_critical_section();
	return 0;
}

int sem_up(sem_t sem)
{
	/* TODO Phase 1 */
	if(sem == NULL)
	{	
		return -1;
	}
	
	enter_critical_section();
	
	
	if(queue_length(sem->waiting) > 0)
	{
		pthread_t TID;
		if(queue_dequeue(sem->waiting,(void**)&TID) == 0)
		{
			thread_unblock(TID);
		}
	}
	sem->internal_count += 1;

	exit_critical_section();
	return 0;
}

int sem_getvalue(sem_t sem, int *sval)
{
	/* TODO Phase 1 */
	if(sem == NULL || sval == NULL)
	{
		return -1;
	}
	if(sem->internal_count > 0)
	{
		 *sval = sem->internal_count;

	}
	if(sem->internal_count == 0)
	{
		*sval = queue_length(sem->waiting)*(-1);
	}
	return 0;	
}

