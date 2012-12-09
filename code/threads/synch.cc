// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
	name = debugName;
	value = initialValue;
	queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
	delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
	//printf("%s: P() value before inter is %d\n",name ,value);
	//printf("currentThread is %s\n", currentThread->getName());
	IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
	while (value == 0) { 			// semaphore not available

		queue->Append((void *)currentThread);	// so go to sleep
		currentThread->Sleep();
	}
	//printf("%s value is %d\n", name, value);

	//printf("value is %d\n", value);

	value--; 					// semaphore available,
	// consume its value
	//    printf("%s segmentation fault?\n",name);
	(void) interrupt->SetLevel(oldLevel);	// re-enable interrupts

}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
	//	printf("%s: V() value before inter is %d\n",name ,value);
	//	printf("currentThread is %s\n", currentThread->getName());
	Thread *thread;
	IntStatus oldLevel = interrupt->SetLevel(IntOff);

	thread = (Thread *)queue->Remove();
	if (thread != NULL)	   // make thread ready, consuming the V immediately
		scheduler->ReadyToRun(thread);
	value++;
	(void) interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!
Lock::Lock(char *lockname) {
	name = lockname;
	threadID = -1;
	locked = false;
	queue = new List();
}

Lock::~Lock() {
	delete queue;
}
void Lock::Acquire() {
	IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
	//	printf("a.1\n");
	while (locked) { 			// if it is locked, sleep.
		//		lockedThread = currentThread;
		//		printf("a.2\n");
		queue->Append((void *)currentThread);
		currentThread->Sleep();
	}
	//	printf("a.3\n");
	threadID = currentThread->getTid();
	locked = true; // if it's not locked, acquire the lock.
	(void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}
void Lock::Release() {
	IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
	if(isHeldByCurrentThread())	{//only the thread that acquired the lock may release it.
		if(!queue->IsEmpty()) {
			scheduler->ReadyToRun((Thread *)queue->Remove());
		}
		locked = false; // release the lock
	}
	(void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}
bool Lock::isHeldByCurrentThread() {
	if(threadID == currentThread->getTid()) {
		return true;
	} else {
		return false;
	}
}

Condition::Condition(char* debugName) {
	name = debugName;
	//	numSleepers = 0;
	queue = new List();
}
Condition::~Condition() {
	delete queue;
}
void Condition::Wait(Lock* conditionLock) {
	IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
	//	numSleepers++;
	//	printf("cv wait 1\n");
	conditionLock->Release();
	queue->Append((void *)currentThread);
	//	printf("cv wait 2\n");
	currentThread->Sleep();
	(void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
	//	printf("cv wait 3\n");
	conditionLock->Acquire();
}
void Condition::Signal(Lock* conditionLock) {
	IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
	//	conditionLock->Release();
	if(!queue->IsEmpty()) {

		scheduler->ReadyToRun((Thread *)queue->Remove());
	}
	(void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}
void Condition::Broadcast(Lock* conditionLock) {
	IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
	//	conditionLock->Release();
	while(!queue->IsEmpty()) {
		//		printf("Broad cast\n");
		scheduler->ReadyToRun((Thread *)queue->Remove());
	}
	(void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}
void Condition::QueuePrint() {
	queue->Mapcar((VoidFunctionPtr)ThreadPrint);
}


Barrier::Barrier(char* debugName, int size) {
	name = debugName;
	barrierSize = size;
	cv = new Condition(name);
	lock = new Lock(name);
	finished = 0;
}
Barrier::~Barrier() {
	delete lock;
	delete cv;
}

void Barrier::Wait() {
	lock->Acquire();
	if(finished != barrierSize-1) {
		finished++;
		cv->Wait(lock);
	} else {
		//		printf("d\n");
		finished = 0;
		cv->Broadcast(lock);
	}
	//	printf("e\n");
	lock->Release();
	//	printf("f\n");

}

void Barrier::Print() {

	printf("Waiting thread...\n");
	cv->QueuePrint();

}
