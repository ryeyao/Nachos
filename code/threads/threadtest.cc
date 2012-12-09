// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "ts.h"
// testnum is set in main.cc
int testnum = 1;
Ts *ts = new Ts();
//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;
    
    for (num = 0;num<100; num++) {
//	currentThread->Yield();

//        currentThread->Sleep();
    }
    ts->PrintThreadInfo();
    printf("*** thread %d looped %d times\n", which, num);
//    currentThread->Sleep();
//    currentThread->Yield();
}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//---------------------------------------------------------------------- 
void ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");
    for(int i = 0;i < 10;++ i) {

        Thread *t = new Thread("forked_t");
	
        t->Fork(SimpleThread,t->getTid());
	//SimpleThread(currentThread->getTid());
    }
    //SimpleThread(currentThread->getTid());
}

//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest(int num)
{
    testnum = num;
    if(num) {
//	printf("testnum = %d\n",num);
	ThreadTest1();
    } else {
	printf("No test specified.\n");
    }
    //printf("Just for debugging.\n");
   /* switch (testnum) {
    case 1:
	ThreadTest1();
	break;
    default:
	printf("No test specified.\n");
	break;
    }
   */	
}

