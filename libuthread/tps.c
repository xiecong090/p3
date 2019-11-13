#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "queue.h"
#include "thread.h"
#include "tps.h"

/* TODO: Phase 2 */

struct queue* tps_queue;

struct page{
	int count;
	void* address;
};

struct tps{
	pthread_t TID;
	struct page* Page;
};

int tid_checker(void* data, void* argv)
{
	struct tps* temp = data;
	pthread_t tid = (*(pthread_t*)argv);
	
	if(tid == temp->TID)
	{
		return 1;
	}
	return 0;
}

int tps_address_checker(void* data, void* argv)
{
	struct tps* temp = data;
	
	if(temp->Page->address == argv)
	{
		return 1;
	}
	return 0;
}

static void segv_handler(int sig, siginfo_t *si, void *context)
{
	/*
	* Get the address corresponding to the beginning of the page where the
	* fault occurred
	*/
	void *p_fault = (void*)((uintptr_t)si->si_addr & ~(TPS_SIZE - 1));
	/*
	* Iterate through all the TPS areas and find if p_fault matches one of them
	*/

	void* address = NULL;
	queue_iterate(tps_queue, tps_address_checker, p_fault, (void**)&address);	

	/* Printf the following error message */
	if (address != NULL)
	{
		fprintf(stderr, "TPS protection error!\n");	
	}
	/* In any case, restore the default signal handlers */
	signal(SIGSEGV, SIG_DFL);
	signal(SIGBUS, SIG_DFL);
	/* And transmit the signal again in order to cause the program to crash */
	raise(sig);
}

int tps_init(int segv)
{
	/* TODO: Phase 2 */

	tps_queue = queue_create();
	if(tps_queue == NULL)
	{
		return -1;			//return -1 if fail to initialize
	}

	if (segv)
	{
		struct sigaction sa;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = SA_SIGINFO;
		sa.sa_sigaction = segv_handler;
		sigaction(SIGBUS, &sa, NULL);
		sigaction(SIGSEGV, &sa, NULL);
	}
	
	return 0;
}

int tps_create(void)
{
	/* TODO: Phase 2 */
	enter_critical_section();
	
	pthread_t running_thread_tid = pthread_self();
	struct tps* tps_check = NULL;
	
	queue_iterate(tps_queue, tid_checker, (void*)running_thread_tid, (void**)&tps_check);
	
	if(tps_check != NULL)
	{
		exit_critical_section();
		return -1;			//return -1 if current thread already has a TPS
	}

	struct tps* TPS = (struct tps*)malloc(sizeof(struct tps));
	TPS->Page = (struct page*)malloc(sizeof(struct page));

	if(TPS == NULL || TPS->Page == NULL)
	{
		exit_critical_section();
		return -1;			//fail to allocate memory
	}
	
	TPS->TID = running_thread_tid;
	
	TPS->Page->address = mmap(NULL, TPS_SIZE, PROT_NONE, MAP_SHARED | MAP_ANON, 0, 0);
	if(TPS->Page->address == MAP_FAILED)
	{
		exit_critical_section();
		return -1;
	}

	TPS->Page->count = 1;
	queue_enqueue(tps_queue, (void*)TPS);
	
	exit_critical_section();
	return 0;
}

int tps_destroy(void)
{
	/* TODO: Phase 2 */
	enter_critical_section();

	pthread_t tid = pthread_self();
	struct tps* target = NULL;
	
	queue_iterate(tps_queue, tid_checker, (void*)tid, (void**)&target);
	
	if(target == NULL)
	{
		exit_critical_section();
		return -1;
	}
	
	munmap(target->Page->address, TPS_SIZE);
	exit_critical_section();
	return 0;
}

int tps_read(size_t offset, size_t length, char *buffer)
{
	/* TODO: Phase 2 */
	enter_critical_section();

	pthread_t tid = pthread_self();
	struct tps* target = NULL;

	int check1, check2;
	
	queue_iterate(tps_queue, tid_checker, (void*)tid, (void**)&target);
	
	if(buffer == NULL || target == NULL || (offset+length) > TPS_SIZE)
	{
		exit_critical_section();
		return -1;
	}
	
	check1 = mprotect(target->Page->address, TPS_SIZE, PROT_READ);
	
	memcpy(buffer, target->Page->address+offset, length);
	
	check2 = mprotect(target->Page->address, TPS_SIZE, PROT_NONE);
	
	if(check1 == -1 || check2 == -1)
	{
		exit_critical_section();
		return -1;
	}
	
	
	exit_critical_section();
	return 0;
}

int tps_write(size_t offset, size_t length, char *buffer)
{
	/* TODO: Phase 2 */
	enter_critical_section();
	pthread_t tid = pthread_self();

	int check1, check2, check3;	
	
	struct tps* target = NULL;
	
	queue_iterate(tps_queue, tid_checker, (void*)tid, (void**)&target);
	
	if(buffer == NULL || target == NULL || (offset+length) > TPS_SIZE)
	{
		exit_critical_section();
		return -1;
	}
	
	if(target->Page->count > 1)
	{
		struct page* new_page = (struct page*)malloc(sizeof(struct page));
		
		if(new_page == NULL)
		{
			exit_critical_section();
			return -1;
		}
		

		new_page->address = mmap(NULL, TPS_SIZE, PROT_READ | PROT_WRITE, 						MAP_ANON | MAP_SHARED , 0, 0);
		
		target->Page->count--;
		
		check1 = mprotect(target->Page->address, TPS_SIZE, PROT_READ | PROT_WRITE);

		memcpy(new_page->address, target->Page->address, TPS_SIZE);

		target->Page = new_page;
	
		check2 = mprotect(target->Page->address, TPS_SIZE, PROT_NONE);

		check3 = mprotect(new_page->address, TPS_SIZE, PROT_NONE);

		if(check1 == -1 || check2 == -1 || check3 == -1)
		{
			exit_critical_section();
			return -1;
		}
	}
	

	check1 = mprotect(target->Page->address, TPS_SIZE, PROT_WRITE);
	
	memcpy(target->Page->address+offset, buffer, length);
	
	check2 = mprotect(target->Page->address, TPS_SIZE, PROT_NONE);
	
	if(check1 == -1 || check2 == -1)
	{
		exit_critical_section();
		return -1;
	}
	
	exit_critical_section();
	return 0;
}

int tps_clone(pthread_t tid)
{
	/* TODO: Phase 2 */
	enter_critical_section();

	pthread_t current_tid = pthread_self();
	struct tps* target = NULL;
	struct tps* caller = NULL;

	
	queue_iterate(tps_queue, tid_checker, (void*)tid, (void**)&target);
	queue_iterate(tps_queue, tid_checker, (void*)current_tid, (void**)&caller);

	if(target == NULL || caller != NULL)
	{
		exit_critical_section();
		return -1;
	}
	
	caller = (struct tps*)malloc(sizeof(struct tps));
	caller->Page = malloc(sizeof(struct page));
	caller->TID = current_tid;
	caller->Page = target->Page;
	caller->Page->count += 1;
	
	queue_enqueue(tps_queue, (void*)caller);
	exit_critical_section();
	return 0;
}

