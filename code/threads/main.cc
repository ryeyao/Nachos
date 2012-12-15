// main.cc 
//	Bootstrap code to initialize the operating system kernel.
//
//	Allows direct calls into internal operating system functions,
//	to simplify debugging and testing.  In practice, the
//	bootstrap code would just initialize data structures,
//	and start a user program to print the login prompt.
//
// 	Most of this file is not needed until later assignments.
//
// Usage: nachos -d <debugflags> -rs <random seed #>
//		-s -x <nachos file> -c <consoleIn> <consoleOut>
//		-f -cp <unix file> <nachos file>
//		-p <nachos file> -r <nachos file> -l -D -t
//              -n <network reliability> -m <machine id>
//              -o <other machine id>
//              -z
//
//    -d causes certain debugging messages to be printed (cf. utility.h)
//    -rs causes Yield to occur at random (but repeatable) spots
//    -z prints the copyright message
//
//  USER_PROGRAM
//    -s causes user programs to be executed in single-step mode
//    -x runs a user program
//    -c tests the console
//
//  FILESYS
//    -f causes the physical disk to be formatted
//    -cp copies a file from UNIX to Nachos
//    -p prints a Nachos file to stdout
//    -r removes a Nachos file from the file system
//    -l lists the contents of the Nachos directory
//    -D prints the contents of the entire file system 
//    -t tests the performance of the Nachos file system
//
//  NETWORK
//    -n sets the network reliability
//    -m sets this machine's host id (needed for the network)
//    -o runs a simple test of the Nachos network software
//    -side c for client, s for server
//  
//
//  NOTE -- flags are ignored until the relevant assignment.
//  Some of the flags are interpreted here; some in system.cc.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#define MAIN
#include "copyright.h"
#undef MAIN

#include "utility.h"
#include "system.h"
#include "time.h"
#ifdef THREADS
extern int testnum;
#endif
#include "synctest.h"
// External functions used by this file

extern void ThreadTest(int), Copy(char *unixFile, char *nachosFile);
extern void Print(char *file), PerformanceTest(void);
extern void StartProcess(char *file), ConsoleTest(char *in, char *out), SynchConsoleTest(char *in, char *out);
extern void MailTest(int networkID);
extern void FakeSocketTest(int networkID);

//----------------------------------------------------------------------
// main
// 	Bootstrap the operating system kernel.  
//	
//	Check command line arguments
//	Initialize data structures
//	(optionally) Call test procedure
//
//	"argc" is the number of command line arguments (including the name
//		of the command) -- ex: "nachos -d +" -> argc = 3 
//	"argv" is an array of strings, one for each command line argument
//		ex: "nachos -d +" -> argv = {"nachos", "-d", "+"}
//----------------------------------------------------------------------

int
main(int argc, char **argv)
{
	int argCount;			// the number of arguments
	// for a particular command

	DEBUG('t', "Entering main");
	(void) Initialize(argc, argv);
	/**
	 * Added by Rye
	 * Set count as a back-up of argc and argValue a back-up of argv.
	 */
	int count = argc;
	char **argValue;
	argValue = argv;
#ifdef THREADS
	//	printf("THREADS is defined.>>>>>>>>>>>>\n");
	for (count--, argValue++; count > 0; count -= argCount, argValue += argCount) {
		argCount = 1;
		//		printf("1argc is %d>>>>>>>>>>>>\n",count);
		switch (argValue[0][1]) {
		case 'q':
			testnum = atoi(argValue[1]);
			argCount++;
			break;
		default:
			testnum = 10;
			break;
		}
	}
	//	printf("2argc is %d>>>>>>>>>>>>\n",count);
	//	ThreadTest(10);
	//testBarrier();
	//producer_consumer_semaphore();
	//producer_consumer_cv_lock();



#endif
	//test time.h
	time_t lt;
	//	struct tm *time_tm;
	//	long int timer_my;
	char *timeString;

	//	timer_my = time(&time_v.tv_sec);
	//	time_tm = localtime(&timer_my);
	//	timeString = asctime(time_tm);
	lt = time(NULL);
	timeString = ctime(&lt);
	printf("****Current time is:%s",timeString);
	count = argc;
	argValue = argv;
	for (count--, argValue++; count > 0; count -= argCount, argValue += argCount) {
		argCount = 1;
		//		printf("Step into the 2nd for.>>>>>>>>>>>>\n");
		if (!strcmp(*argValue, "-z"))               // print copyright
			printf("Print copyright\n");
		printf (copyright);
#ifdef USER_PROGRAM
		if (!strcmp(*argValue, "-x")) {        	// run a user program
			ASSERT(count > 1);
			StartProcess(*(argValue + 1));
			argCount = 2;
		} else if (!strcmp(*argValue, "-c")) {      // test the console
			if (count == 1) {
//				char *input = "Test console input.";
				ConsoleTest(NULL, NULL);
			} else {
				ASSERT(count > 2);
				ConsoleTest(*(argValue + 1), *(argValue + 2));
				argCount = 3;
			}
			printf("Waiting for commands...\n");
			interrupt->Halt();		// once we start the console, then
			// Nachos will loop forever waiting
			// for console input
		} else if (!strcmp(*argValue, "-sc")) {      // test the synchConsole
			if (count == 1) {
				SynchConsoleTest(NULL, NULL);
			} else {
				ASSERT(count > 2);
				SynchConsoleTest(*(argValue + 1), *(argValue + 2));
				argCount = 3;
			}
			printf("(Synchronously)Waiting for commands...\n");
			interrupt->Halt();		// once we start the console, then
			// Nachos will loop forever waiting
			// for console input
		}
#endif // USER_PROGRAM
		//#ifndef FILESYS
		//#define FILESYS
		//#endif
#ifdef FILESYS
		//		printf("Step into FILESYS, argc is %d>>>>>>>>>>>>>>>\n",count);
		if (!strcmp(*argValue, "-cp")) { 		// copy from UNIX to Nachos
			ASSERT(count > 2);
			Copy(*(argValue + 1), *(argValue + 2));
			argCount = 3;
		} else if (!strcmp(*argValue, "-p")) {	// print a Nachos file
			ASSERT(count > 1);
			Print(*(argValue + 1));
			argCount = 2;
		} else if (!strcmp(*argValue, "-r")) {	// remove Nachos file
			ASSERT(count > 1);
			fileSystem->Remove(*(argValue + 1));
			argCount = 2;
		} else if (!strcmp(*argValue, "-l")) {	// list Nachos directory
			fileSystem->List();
		} else if (!strcmp(*argValue, "-D")) {	// print entire filesystem
			printf("\nbegin\n");
			fileSystem->Print();
			printf("done??\n");
		} else if (!strcmp(*argValue, "-t")) {	// performance test
			PerformanceTest();
		}
#endif // FILESYS
#ifdef NETWORK
		if (!strcmp(*argValue, "-o")) {
			ASSERT(count > 1);
			Delay(2); 				// delay for 2 seconds
			// to give the user time to
			// start up another nachos
			//MailTest(atoi(*(argValue + 1)));
			FakeSocketTest(atoi(*(argValue + 1)));
			argCount = 2;
		}
#endif // NETWORK
	}

	currentThread->Finish();	// NOTE: if the procedure "main"
	// returns, then the program "nachos"
	// will exit (as any other normal program
	// would).  But there may be other
	// threads on the ready list.  We switch
	// to those threads by saying that the
	// "main" thread is finished, preventing
	// it from returning.
	return(0);			// Not reached...
}
