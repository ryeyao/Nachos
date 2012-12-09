/* halt.c
 *	Simple program to test whether running a user program works.
 *	
 *	Just do a "syscall" that shuts down the OS.
 *
 * 	NOTE: for some reason, user programs with global data structures 
 *	sometimes haven't worked in the Nachos environment.  So be careful
 *	out there!  One option is to allocate data structures as 
 * 	automatics within a procedure, but if you do this, you have to
 *	be careful to allocate a big enough stack to hold the automatics!
 */

#include "syscall.h"

int
main()
{
	//Test Create
//	char *fname = "TestSysCall";
//	int fdw,fdr;
//	char *content = "Test my system call.";
//	char buffer[100];
//	int readSize;
//	//	Create("TestUsrProg/TestSysCall");
//	//	fd = Open("TestUsrProg/TestSysCall");
//	Create(fname);
//	fdw = Open(fname);
//	Write(content,21,fdw);
//	Close(fdw);

//	fdr = Open(fname);
//	//	Write("Test my system call.", 21, fd);
//	//
//	readSize = Read(buffer, 21, fd);
//	Close(fd);
//	Print("Bytes read is %d",readSize);
	Halt();
	/* not reached */
}
