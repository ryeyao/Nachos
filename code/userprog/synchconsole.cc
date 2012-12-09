/*
 * synchconsole.cc
 *
 *  Created on: 2012-11-18
 *      Author: rye
 */
// console.cc
//	Routines to simulate a serial port to a console device.
//	A console has input (a keyboard) and output (a display).
//	These are each simulated by operations on UNIX files.
//	The simulated device is asynchronous,
//	so we have to invoke the interrupt handler (after a simulated
//	delay), to signal that a byte has arrived and/or that a written
//	byte has departed.
//
//  DO NOT CHANGE -- part of the machine emulation
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synchconsole.h"
#include "system.h"

//// Dummy functions because C++ is weird about pointers to member functions
static void SynchConsoleRead(int c)
{
	SynchConsole *synchConsole = (SynchConsole *)c;
	synchConsole->CheckCharAvail();
}
static void SynchConsoleWrite(int c)
{
	SynchConsole *synchConsole = (SynchConsole *)c;
	synchConsole->WriteDone();
}

//----------------------------------------------------------------------
// Console::Console
// 	Initialize the simulation of a hardware console device.
//
//	"readFile" -- UNIX file simulating the keyboard (NULL -> use stdin)
//	"writeFile" -- UNIX file simulating the display (NULL -> use stdout)
// 	"readAvail" is the interrupt handler called when a character arrives
//		from the keyboard
// 	"writeDone" is the interrupt handler called when a character has
//		been output, so that it is ok to request the next char be
//		output
//----------------------------------------------------------------------

SynchConsole::SynchConsole(char *readFile, char *writeFile)
{
	writeSemaphore = new Semaphore("Synch Console Write", 0);
	readSemaphore = new Semaphore("Sync Console Avail", 0);
	putLock = new Lock("Synch Console Put Lock");
	getLock = new Lock("Synch Console Get Lock");
	console = new Console(readFile, writeFile, SynchConsoleRead, SynchConsoleWrite, (int)this);
}

//----------------------------------------------------------------------
// Console::~Console
// 	Clean up console emulation
//----------------------------------------------------------------------

SynchConsole::~SynchConsole()
{
	delete console;
	delete getLock;
	delete putLock;
	delete readSemaphore;
	delete writeSemaphore;
}

//----------------------------------------------------------------------
// Console::CheckCharAvail()
// 	Periodically called to check if a character is available for
//	input from the simulated keyboard (eg, has it been typed?).
//
//	Only read it in if there is buffer space for it (if the previous
//	character has been grabbed out of the buffer by the Nachos kernel).
//	Invoke the "read" interrupt handler, once the character has been
//	put into the buffer.
//----------------------------------------------------------------------

void
SynchConsole::CheckCharAvail()
{
	readSemaphore->V();
}

//----------------------------------------------------------------------
// Console::WriteDone()
// 	Internal routine called when it is time to invoke the interrupt
//	handler to tell the Nachos kernel that the output character has
//	completed.
//----------------------------------------------------------------------

void
SynchConsole::WriteDone()
{
	writeSemaphore->V();
}

//----------------------------------------------------------------------
// Console::GetChar()
// 	Read a character from the input buffer, if there is any there.
//	Either return the character, or EOF if none buffered.
//----------------------------------------------------------------------

char
SynchConsole::GetChar()
{
	getLock->Acquire();
	readSemaphore->P();
	char ch = console->GetChar();
	getLock->Release();
	return ch;
}

//----------------------------------------------------------------------
// Console::PutChar()
// 	Write a character to the simulated display, schedule an interrupt
//	to occur in the future, and return.
//----------------------------------------------------------------------

void
SynchConsole::PutChar(char ch)
{
	putLock->Acquire();
	console->PutChar(ch);
	writeSemaphore->P();
	putLock->Release();
}


