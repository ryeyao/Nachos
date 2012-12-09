/*
 * syncmutexproblems.cc
 *
 *  Created on: 2012-10-17
 *      Author: rye
 */
#include "copyright.h"
#include "synctest.h"
#include "ts.h"
#include "system.h"
Condition *cv_fill = new Condition("cv_producer");
Condition *cv_empty = new Condition("cv_consumer");
Lock *lock_cv_pc = new Lock("lock_cv_pc");
int cv_fillcount = 0;
int cv_emptycount = 10;

Semaphore *mutex = new Semaphore("mutex",1);
Semaphore *fillcount = new Semaphore("fillcount",0);
Semaphore *empty = new Semaphore("empty",20);

Barrier b("test",3);
Ts *ts1 = new Ts();

void producer_cv_lock(int arg) {

	//int i = 10;
	while(arg--) {
		lock_cv_pc->Acquire();
		if(cv_emptycount == 0) {//buffer is full
			printf("Buffer is full! Waitting for an empty space\n");
			cv_empty->Wait(lock_cv_pc);
		}
		cv_fillcount++;
		cv_emptycount--;
			printf("Producer_cv: produce an item.\n");
			printf("\t%d items in buffer\n",cv_fillcount);
//			ts1->PrintThreadInfo();
			lock_cv_pc->Release();
			cv_fill->Signal(lock_cv_pc);

		//currentThread->Yield();
	}
}
void consumer_cv_lock(int arg) {

		//int i = 10;
	while(arg--) {
		lock_cv_pc->Acquire();
		if(cv_fillcount == 0) {//buffer is empty
			printf("Buffer is empty! Waiting for an item\n");
			cv_fill->Wait(lock_cv_pc);
//			ts1->PrintThreadInfo();
		}
		cv_fillcount--;
		cv_emptycount++;
		printf("Consumer_cv: consume an item.\n");
		printf("\t%d items left\n",cv_fillcount);
//		ts1->PrintThreadInfo();
		lock_cv_pc->Release();
		cv_empty->Signal(lock_cv_pc);
		//currentThread->Yield();
	}
}
void producer_semaphore(int arg) {

	//int arg = 10;
	while(arg--) {
		//produce an item
		printf("Producer: produce an item.\n");
//		ts1->PrintThreadInfo();
		empty->P();//wait for an empty space
//		ts1->PrintThreadInfo();
			mutex->P();//wait for mutex
				//put item into buffer
				printf("Producer: put item into buffer.\n");
//				ts1->PrintThreadInfo();
			mutex->V();
		fillcount->V();
	}
}

void consumer_semaphore(int arg) {

		//int arg = 10;
	while(arg--) {
		fillcount->P();
//		ts1->PrintThreadInfo();
		mutex->P();
		//get an item from buffer
			printf("Consumer: get an item from buffer.\n");
//			ts1->PrintThreadInfo();
			mutex->V();
		empty->V();
		//consume this item
		printf("Consumer: cocnsume an item.\n");
	}
}


void producer_consumer_cv_lock() {
	/**
	 * Producer/consumer problem
	 */
	//using locks and CVs

	Thread *tproducer = new Thread("producer");
	Thread *tconsumer = new Thread("consumer");
	Thread *tproducer2 = new Thread("producer2");
	Thread *tconsumer2 = new Thread("consumer2");
	tproducer->Fork(producer_cv_lock,100);
	tconsumer->Fork(consumer_cv_lock,100);
	tproducer2->Fork(producer_cv_lock,100);
	tconsumer2->Fork(consumer_cv_lock,100);
}
void producer_consumer_semaphore() {
	/**
	 * Producer/consumer problem
	 */
	//using semaphore

	Thread *sema_producer = new Thread("s_producer");
	Thread *sema_consumer = new Thread("s_consumer");
	Thread *sema_producer2 = new Thread("s_producer2");
	Thread *sema_consumer2 = new Thread("s_consumer2");
	sema_producer->Fork(producer_semaphore,10);
	sema_consumer->Fork(consumer_semaphore,10);
	sema_producer2->Fork(producer_semaphore,1);
	sema_consumer2->Fork(consumer_semaphore,1);
}

void testBarrierThread(int arg) {
	Barrier *b = (Barrier *)arg;
	for(int i=0;i<6;i++) {
		printf("%s prints %d\n",currentThread->getName(),i);
		b->Print();
		b->Wait();
	}
		//printf("testBarrier #%d is running",arg);
}
void testBarrier() {
	Thread *t1 = new Thread("test1");
	Thread *t2 = new Thread("test2");
	Thread *t3 = new Thread("test3");

	t1->Fork((VoidFunctionPtr)testBarrierThread,(int)&b);
	t2->Fork((VoidFunctionPtr)testBarrierThread,(int)&b);
	t3->Fork((VoidFunctionPtr)testBarrierThread,(int)&b);
}


