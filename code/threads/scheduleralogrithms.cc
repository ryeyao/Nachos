/*
 * scheduleralogrithms.cc
 *
 *  Created on: 2012-10-19
 *      Author: rye
 */
#include "scheduleralogrithms.h"
#include "list.h"
#include "system.h"
int d = 2;
void Scheduler_RR(int delay) {
	if(interrupt->getStatus() == IdleMode) {
		// Do nothing
		return;
	}
//	printf("%s:tid is %d, priority is %d,slice left is %d\n", currentThread->getName(),currentThread->getTid(),currentThread->getPriority(),currentThread->getTimeSlice());
	//if(d--) return;
	//else d = 2;
	int slicesLeft = currentThread->getTimeSlice();
	int priority = currentThread->getPriority();
	List *readyList = scheduler->GetReadyList();

	if(slicesLeft--) {
//		printf("Has slices left\n");
		currentThread->setTimeSlice(slicesLeft);
		return;
	}
	if(slicesLeft <= 0) {
//		printf("No more slices left yield\n");
		currentThread->setDefaultTimeSlice();
		interrupt->YieldOnReturn();

	}

	currentThread->setPriority(++priority);
	if(priority >= 255) {
//		printf("Priority is out of range\n");
		scheduler->AdjustAllPriority();
	}
	if(!readyList->IsEmpty() && priority < ((Thread *)readyList->Front()->item)->getPriority()) {
		//Do nothing
//		printf("Current thread has the highest priority\n");
		return;
	} else {
//		printf("There is a thread with higher priority\n");
		interrupt->YieldOnReturn();
	}
}


