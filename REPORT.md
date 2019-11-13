#Project 3
#Cong Xie, 

###Phase 1. semaphore API
first of all, we started with a semaphore struct which contained a count
(determine the number of available resources) and a queue(store the threads).
```
struct semaphore {
	int internal_count;
	struct queue* waiting;
};
```
``sem_t sem_create(size_t count)``
this function is very straight forward. just initalized the the
struct and allocated memory for it, and report errors if there were any.

``int sem_destroy(sem_t sem)``
deallocated memory when there is no thread in the waiting queue. otherwise,
report error.

``int sem_down(sem_t sem)``
the sem_down function is the first time we use critical section to achieve 
mutual exclusion. since we use thread_block and thread_unblock as the 
spinlock, we simply block the current thread when count is zero(which 
means no resource is available) and put the thread into the waiting 
queue to wait unblock. additonally, we check the completeness of the
thread. we only block the thread after we put it in the waiting queue.

``int sem_up(sem_t sem)``
very similar to sem_down. use critical section to achieve mutual exclusion.
we check if there is any thread in the waiting queue and unblock it(release
a resource to the semaphore).

``int sem_getvalue(sem_t sem, int *sval)``
assign the internal count of semapher to sval so we can inspect the condition 
of the semaphore.

###Phase 2. TPS
In this phase we created two structs:
```
struct page{
	int count;
	void* address;
};
```

```
struct tps{
	pthread_t TID;
	struct page* Page;
};
```
the tps struct contains the tid of the thread and the pointer of its page.
the page struct contains a count(let us know how many threads share this page)
and the address of the page.

```
struct queue* tps_queue;
```
we use a global queue to store the TPS so that we can use queue_iterate to 
find the required thread.

``int tid_checker(void* data, void* argv)``
a function works with queue_iterate. help us to check if the thread already has
TPS.

``int tps_address_checker(void* data, void* argv)``
a function works with queue_iterate. help us to check if the memory protection 
works.

``int tps_init(int segv)``
initialize the TPS API, queue, and a handler. only call once.

``int tps_create(void)``
create a tps and link it with current thread if the thread does not have one.
we use mmap to create a specific size of space, and then we enqueue it to the
tps_queue.

``int tps_destroy(void)``
destroy the current thread's TPS if it has one. otherwise, return -1;

``int tps_read(size_t offset, size_t length, char *buffer)``
read required size of data from the current thread and put them into buffer.
first, we find the current thread's TPS if it has one. then, we change its 
memory protection rule to PROT_READ(so we can read it). copy the required
length of data to buffer, and change its rule back to PRON_NONE(can not be 
accessed). by doing this, we can ensure the sucurity(the page's space can
only be accessed in the critical section).

``int tps_write(size_t offset, size_t length, char *buffer)``
similar to tps_read. but we have to deal with CoW. When write to a page that
shared by more than one TPS, we first create a new page and copy the current
thread's page to it. this new page would be the private copy of the calling
thread. we set its memory protection rule to PRON_WRITE during the writing
operation and change back to PRON_NONE when we finish it.

``int tps_clone(pthread_t tid)``
this function create a new TPS for the calling thread and refer to the target
thread's page. 

##test file
passed all existing test files, but unable to come up with own. 

##resource
slide, wikipedia, man page, and CS tutoring.


