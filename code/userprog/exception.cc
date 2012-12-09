// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "openfile.h"
#include "string.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
	int type = machine->ReadRegister(2);

	if(which == SyscallException) {
		if (type == SC_Halt) {
			DEBUG('a', "Shutdown, initiated by user program.\n");
			printf("Halt:Shutdown\n");
			interrupt->Halt();

		} else if (type == SC_Create) {
			DEBUG('a', "Create a new file.\n");
			int baseAddr = machine->ReadRegister(4);
			int value = -1;
			char *name;

			int size = 0;
			while(value != 0) {
				machine->ReadMem(baseAddr + size, 1,&value);
				size++;
			}
			name = new char[size];
			name[--size] = '\0';
			int counter = 0;
			while(size--) {
				machine->ReadMem(baseAddr++,1,&value);
				name[counter++] = (char)value;
			}
			printf("Exception: First arg is %s. Arg's length is %d\n",name,counter + 1);
			if(!fileSystem->Create(name, DT_NORMAL)) {
				printf("Exception: Create file failed.\n");
			}
			delete name;

		} else if (type == SC_Open) {
			DEBUG('a', "Open a file. Return 0 if failed.\n");
			int baseAddr = machine->ReadRegister(4);
			int value = -1;
			char *name;

			int size = 0;
			while(value != 0) {
				machine->ReadMem(baseAddr + size, 1,&value);
				size++;
			}
			name = new char[size];
			name[--size] = '\0';
			int counter = 0;
			while(size--) {
				machine->ReadMem(baseAddr++,1,&value);
				name[counter++] = (char)value;
			}
			OpenFile* file = fileSystem->Open(name);
			//			OpenFileId fd = (int)file;
			OpenFileId fd = file->GetFileDescriptor();
#ifdef FILESYS_STUB
#else//FILESYS
			fileSystem->AddToTable(fd,file);
#endif
			//			if(file) {
			//				fd = (int)file;
			//			} else {
			//				fd = -1;
			//			}
			machine->WriteRegister(2,fd);
			delete name;

		} else if (type == SC_Close) {
			DEBUG('a', "Close a file specified by id.\n");
			int fd = machine->ReadRegister(4);

#ifdef FILESYS_STUB
			OpenFile* file = new OpenFile(fd);
			//			OpenFile* file = fileSystem->GetFromTable(fd);
			delete file;
#else//FILESYS
			OpenFile* file = fileSystem->GetFromTable(fd);
			delete file;
			fileSystem->RemoveFromTable(fd);
#endif

		} else if (type == SC_Write) {
			DEBUG('a', "Write file.\n");
			int fd = machine->ReadRegister(6);
			int size = machine->ReadRegister(5);
			int baseAddr = machine->ReadRegister(4);
			char* buffer = new char[size];
			//			int value = -1;
			int i = 0;
			while(i < size) {
				machine->ReadMem(baseAddr + i, 1,(int *)&buffer[i]);
				i++;
			}
#ifdef FILESYS_STUB
			OpenFile* file = new OpenFile(fd);
#else//FILESYS
			OpenFile* file = fileSystem->GetFromTable(fd);
#endif
			int realSize = file->Write(buffer,size);
			if(realSize != size) {
				printf("Exception: Only wrote %d bytes of size.\n",realSize,size);
			} else {
				printf("Exception: Write %d bytes successfully\n",realSize);
			}
			delete buffer;

		} else if (type == SC_Read) {
			DEBUG('a', "Read a file.\n");
			int fd = machine->ReadRegister(6);
			int size = machine->ReadRegister(5);
			int baseAddr = machine->ReadRegister(4);
			char* buffer = new char[size];
#ifdef FILESYS_STUB
			OpenFile* file = new OpenFile(fd);
#else//FILESYS
			OpenFile* file = fileSystem->GetFromTable(fd);
#endif
			//			file->Seek(0);
			int realSize = file->Read(buffer,size);
			int i = 0;
			while(i < size) {
				machine->WriteMem(baseAddr + i, 1,(int)buffer);
				i++;
			}
			if(realSize != size) {
				printf("Exception: Only wrote %d bytes of size.\n",realSize,size);
			} else {
				machine->WriteRegister(2, realSize);
				printf("Exception: Read %d bytes successfully\n",realSize);
			}
			delete buffer;
		} else if (type == SC_Print) {//Print somethine...
			DEBUG('a', "Print a string within a integer.\n");
			int baseAddr = machine->ReadRegister(4);
			int arg2 = machine->ReadRegister(5);
			int value = -1;
			char *content;
			int size = 0;
			while(value != 0) {
				machine->ReadMem(baseAddr + size, 1,&value);
				size++;
			}
			content = new char[size];
			content[--size] = '\0';
			int counter = 0;
			while(size--) {
				machine->ReadMem(baseAddr++,1,&value);
				content[counter++] = (char)value;
			}
			printf(content,arg2);
			delete content;
		} else if (type == SC_Mkdir) {
			DEBUG('a', "Make a directory specified by path.\n");
			int baseAddr = machine->ReadRegister(4);
			int value = -1;
			char *name;

			int size = 0;
			while(value != 0) {
				machine->ReadMem(baseAddr + size, 1,&value);
				size++;
			}
			name = new char[size];
			name[--size] = '\0';
			int counter = 0;
			while(size--) {
				machine->ReadMem(baseAddr++,1,&value);
				name[counter++] = (char)value;
			}
			if(!fileSystem->Create(name,DT_DIR)) {
				printf("Exception: Create directory %s failed.\n",name);
			} else {
				printf("Exception: Directory %s created successfully.\n",name);
			}
			delete name;
		}else {
			printf("Exception: Unexpected exception type %d\n", type);
			ASSERT(FALSE);
		}

	} else {
		printf("Exception: Unexpected mode %d\n", which);
		ASSERT(FALSE);
	}
}
