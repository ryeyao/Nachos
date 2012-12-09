// filesys.cc 
//	Routines to manage the overall operation of the file system.
//	Implements routines to map from textual file names to files.
//
//	Each file in the file system has:
//	   A file header, stored in a sector on disk 
//		(the size of the file header data structure is arranged
//		to be precisely the size of 1 disk sector)
//	   A number of data blocks
//	   An entry in the file system directory
//
// 	The file system consists of several data structures:
//	   A bitmap of free disk sectors (cf. bitmap.h)
//	   A directory of file names and file headers
//
//      Both the bitmap and the directory are represented as normal
//	files.  Their file headers are located in specific sectors
//	(sector 0 and sector 1), so that the file system can find them 
//	on bootup.
//
//	The file system assumes that the bitmap and directory files are
//	kept "open" continuously while Nachos is running.
//
//	For those operations (such as Create, Remove) that modify the
//	directory and/or bitmap, if the operation succeeds, the changes
//	are written immediately back to disk (the two files are kept
//	open during all this time).  If the operation fails, and we have
//	modified part of the directory and/or bitmap, we simply discard
//	the changed version, without writing it back to disk.
//
// 	Our implementation at this point has the following restrictions:
//
//	   there is no synchronization for concurrent accesses
//	   files have a fixed size, set when the file is created
//	   files cannot be bigger than about 3KB in size
//	   there is no hierarchical directory structure, and only a limited
//	     number of files can be added to the system
//	   there is no attempt to make the system robust to failures
//	    (if Nachos exits in the middle of an operation that modifies
//	    the file system, it may corrupt the disk)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "disk.h"
#include "bitmap.h"
#include "directory.h"
#include "filehdr.h"
#include "filesys.h"
// Sectors containing the file headers for the bitmap of free sectors,
// and the directory of files.  These file headers are placed in well-known 
// sectors, so that they can be located on boot-up.
#define FreeMapSector 		0
#define DirectorySector 	1

// Initial file sizes for the bitmap and directory; until the file system
// supports extensible files, the directory size sets the maximum number 
// of files that can be loaded onto the disk.
#define FreeMapFileSize 	(NumSectors / BitsInByte)
#define NumDirEntries 		10
#define MaxDirEntries			128
#define DirectoryFileSize 	(sizeof(DirectoryEntry) * NumDirEntries)

//----------------------------------------------------------------------
// FileSystem::FileSystem
// 	Initialize the file system.  If format = TRUE, the disk has
//	nothing on it, and we need to initialize the disk to contain
//	an empty directory, and a bitmap of free sectors (with almost but
//	not all of the sectors marked as free).  
//
//	If format = FALSE, we just have to open the files
//	representing the bitmap and the directory.
//
//	"format" -- should we initialize the disk?
//----------------------------------------------------------------------
OpenFile* currentFreeMapFile = NULL;
OpenFile* rootDirectoryFile = NULL;// "Root" directory -- list of
// file names, represented as a file
OpenFile* currentDirectoryFile = NULL;

FileSystem::FileSystem(bool format)
{ 
	DEBUG('f', "Initializing the file system.\n");
	if (format) {
		BitMap *freeMap = new BitMap(NumSectors);
		Directory *directory = new Directory(DirectorySector);
		FileHeader *mapHdr = new FileHeader;
		FileHeader *rootDirHdr = new FileHeader;

		DEBUG('f', "Formatting the file system.\n");
		//		remove("DISK");
		// First, allocate space for FileHeaders for the directory and bitmap
		// (make sure no one else grabs these!)
		freeMap->Mark(FreeMapSector);
		freeMap->Mark(DirectorySector);

		// Second, allocate space for the data blocks containing the contents
		// of the directory and bitmap files.  There better be enough space!

		ASSERT(mapHdr->Allocate(freeMap, DT_DISKBITMAP, -1));
		ASSERT(rootDirHdr->Allocate(freeMap, DT_DIR, -1));

		// Flush the bitmap and directory FileHeaders back to disk
		// We need to do this before we can "Open" the file, since open
		// reads the file header off of disk (and currently the disk has garbage
		// on it!).

		DEBUG('f', "Writing headers back to disk.\n");
		mapHdr->WriteBack(FreeMapSector);
		rootDirHdr->WriteBack(DirectorySector);

		// OK to open the bitmap and directory files now
		// The file system operations assume these two files are left open
		// while Nachos is running.

		freeMapFile = new OpenFile(FreeMapSector);
		directoryFile = new OpenFile(DirectorySector);
		currentFreeMapFile = freeMapFile;
		// Once we have the files "open", we can write the initial version
		// of each file back to disk.  The directory at this point is completely
		// empty; but the bitmap has been changed to reflect the fact that
		// sectors on the disk have been allocated for the file headers and
		// to hold the file data for the directory and bitmap.

		DEBUG('f', "Writing bitmap and directory back to disk.\n");
		freeMap->WriteBack(freeMapFile);	 // flush changes to disk
		directory->WriteBack(directoryFile);
		//test
		//		printf("Test before fetch\n");
		//		freeMap->Print();
		//		directory->Print();
		//		mapHdr->Print();
		//		dirHdr->Print();
		//
		//		mapHdr->FetchFrom(FreeMapSector);
		//		dirHdr->FetchFrom(DirectorySector);
		//		freeMap->FetchFrom(freeMapFile);	 // flush changes to disk
		//		directory->FetchFrom(directoryFile);
		//		printf("Test after fetch\n");
		//		freeMap->Print();
		//		directory->Print();
		//		mapHdr->Print();
		//		dirHdr->Print();



		if (DebugIsEnabled('f')) {
			freeMap->Print();
			directory->Print();

			delete freeMap;
			delete directory;
			delete mapHdr;
			delete rootDirHdr;
		}
	} else {
		// if we are not formatting the disk, just open the files representing
		// the bitmap and directory; these are left open while Nachos is running
		freeMapFile = new OpenFile(FreeMapSector);
		directoryFile = new OpenFile(DirectorySector);
		currentFreeMapFile = freeMapFile;
		currentDirectoryFile = directoryFile;
		rootDirectoryFile = directoryFile;
	}
}

/**
 * Added by Rye
 */
int FileSystem::ParseDirectory(char* name) {//find the directory the file is in, if the file is a directory, return it's father directory

	char *finalName = new char[strlen(name)+1];
	char *temp = new char[strlen(name)+1];
	OpenFile* tempDirFile;//initially to be the current directory
	Directory* dir = new Directory(DirectorySector);
	int sector = DirectorySector;//initially to be root

	if(name[0] == '/') {//Absolute path
		strcpy(finalName, (char *)(name + 1));//remove the first '/'
		strcpy(name, finalName);
		tempDirFile = new OpenFile(DirectorySector);//change the ptr to root directory, parse start from root
	} else {
		strcpy(finalName, name);
		tempDirFile = currentDirectoryFile;//not absolute path, parse start from current directory
	}
	dir->FetchFrom(tempDirFile);//fetch the directory content from disk. This is the directory where the file is to be located.

	int i = 1;//counter for counting slashes.
	if(strrchr(finalName, '/') && ((char)*strrchr(finalName, '/') == name[strlen(name) - 1])) {//name ends with slash(es).
		finalName[strlen(finalName) - 1] = '\0';//remove the last '/'
	}



	if(!strchr(finalName, '/')) {//No more slashes among the file path implicates that file is in root directory
		strcpy(name, finalName);
		if(tempDirFile != currentDirectoryFile && tempDirFile != rootDirectoryFile) {
			delete tempDirFile;
		}
		printf("File [%s] was found or should be created in [root(%d)]\n",name,sector);
		delete dir;
		delete temp;
		delete finalName;
		return sector;
	}

	while(strchr(finalName, '/')) {//If file is not in root then it must be in one of the root's child directory.
		//Note: assume there is no more than one slash joined together.
		strncpy(temp, finalName, strchr(finalName, '/') - finalName);//find the first directory in the path.
		temp[strchr(finalName, '/') - finalName] = '\0';

		strcpy(finalName, (char *)(strchr(finalName, '/') + 1));//cut the first directory from finalName.
		sector = dir->Find(temp);//see if the first directory exists (its father directory)

		if(sector < 0 || !dir->IsDirectory(temp)) {//if not
			if(tempDirFile != currentDirectoryFile && tempDirFile != rootDirectoryFile) {
				delete tempDirFile;
			}
			printf("File [%s] is not a directory or Directory [%s] does not exist.\n",temp,temp);
			delete dir;
			delete temp;
			delete finalName;
			return -1;
		}

		if(tempDirFile->GetFileDescriptor() != currentDirectoryFile->GetFileDescriptor() && tempDirFile->GetFileDescriptor() != rootDirectoryFile->GetFileDescriptor()) {//deallocate the buffer for dir
			delete tempDirFile;
		}
		//		dirHdr->FetchFrom(sector);
		tempDirFile = new OpenFile(sector);
		dir->FetchFrom(tempDirFile);//step into next directory and loop again.
	}
	//	if(!strcmp(finalName, temp)) {//is directory, return its father
	//		printf("Could not open a directory file for write for now...\n");
	//		return -1;
	//	}
	//	if(dir->Find(finalName) >= 0) {
	//		if(tempDirFile != currentDirectoryFile && tempDirFile != rootDirectoryFile) {
	//			delete tempDirFile;
	//		}
	//		strcpy(name, finalName);
	//		printf("File %s already exists.\n",finalName);
	//		return -1;
	//	}

	strcpy(name, finalName);
	printf("File [%s] was found or should be created in [%s(%d)]\n",name,temp,sector);
	if(tempDirFile->GetFileDescriptor() != currentDirectoryFile->GetFileDescriptor() && tempDirFile->GetFileDescriptor() != rootDirectoryFile->GetFileDescriptor()) {//deallocate the buffer for dir
		delete tempDirFile;
	}
	delete dir;
	delete temp;
	delete finalName;
	return sector;//return the directory sector where the file is to be created.
}
//----------------------------------------------------------------------
// FileSystem::Create
// 	Create a file in the Nachos file system (similar to UNIX create).
//	Since we can't increase the size of files dynamically, we have
//	to give Create the initial size of the file.
//
//	The steps to create a file are:
//	  Make sure the file doesn't already exist
//        Allocate a sector for the file header
// 	  Allocate space on disk for the data blocks for the file
//	  Add the name to the directory
//	  Store the new file header on disk 
//	  Flush the changes to the bitmap and the directory back to disk
//
//	Return TRUE if everything goes ok, otherwise, return FALSE.
//
// 	Create fails if:
//   		file is already in directory
//	 	no free space for file header
//	 	no free entry for file in directory
//	 	no free space for data blocks for the file 
//
// 	Note that this implementation assumes there is no concurrent access
//	to the file system!
//
//	"name" -- name of file to be created
//	"initialSize" -- size of file to be created
//----------------------------------------------------------------------
// Open for writing. When dynamically create file, no initialSize is needed.
bool
FileSystem::Create(char *name, int fileType)
{
	Directory *directory;
	BitMap *freeMap;
	FileHeader *hdr;
	char* nameBuffer;
	int sector;
	bool success;

	//	fileType = 0;
	DEBUG('f', "Creating file %s, type [%d]\n", name, fileType);
	nameBuffer = new char[strlen(name) + 1];
	strcpy(nameBuffer, name);
	int dirSector = ParseDirectory(nameBuffer);
	OpenFile *dirFile = new OpenFile(dirSector);
	directory = new Directory(dirSector);
	directory->FetchFrom(dirFile);//get the final directory from path name
	//	FileHeader dirHdr = new FileHeader;

	if (directory->Find(nameBuffer) != -1)
		success = FALSE;			// file is already in directory
	else {
		freeMap = new BitMap(NumSectors);
		freeMap->FetchFrom(freeMapFile);
		sector = freeMap->Find();	// find a sector to hold the file header
		if (sector == -1)
			success = FALSE;		// no free block for file header
		else if (!directory->Add(nameBuffer, sector, fileType))
			success = FALSE;	// no space in directory
		else {
			hdr = new FileHeader;
			if (!hdr->Allocate(freeMap, fileType, dirSector))
				success = FALSE;	// no space on disk for data
			else {
				success = TRUE;
				printf("File [%s(%d):%d bytes] created in directory [%d]\n",nameBuffer,sector,hdr->FileLength(),dirSector);
				//				printf("File %s wroten at No.%d sector\n",name,sector);
				// everthing worked, flush all changes back to disk
				hdr->WriteBack(sector);
				delete hdr;
				hdr = new FileHeader;
				hdr->FetchFrom(sector);
				freeMap->WriteBack(freeMapFile);
				directory->WriteBack(dirFile);//write back the directory where the new created file exists
				printf("Directory [%d] is just written back with [length %d].\n",dirSector,dirFile->Length());

			}
			delete hdr;
		}
		delete freeMap;
	}
	if(dirFile->GetFileDescriptor() == rootDirectoryFile->GetFileDescriptor()) {//deallocate the buffer for dir
		printf("Directory is system directory, delete it and create again!\n");
		delete dirFile;//write file header back to disk
		//		delete rootDirectoryFile;
		//		dirFile = new OpenFile(dirSector);
		//		//				currentDirectoryFile = dirFile;
		//		rootDirectoryFile = dirFile;
	} else {
		delete dirFile;
	}
	delete directory;
	delete nameBuffer;
	return success;
}

//----------------------------------------------------------------------
// FileSystem::Open
// 	Open a file for reading and writing.  
//	To open a file:
//	  Find the location of the file's header, using the directory 
//	  Bring the header into memory
//
//	"name" -- the text name of the file to be opened
//----------------------------------------------------------------------

OpenFile *
FileSystem::Open(char *name)
{ 
	char* nameBuffer;
	Directory *directory;
	OpenFile *openFile = NULL;
	OpenFile *dirFile = NULL;
	int sector, dirSector;

	DEBUG('f', "Opening file [%s]\n", name);
	nameBuffer = new char[strlen(name) + 1];
	strcpy(nameBuffer, name);
	dirSector = ParseDirectory(nameBuffer);
	directory = new Directory(dirSector);
	dirFile = new OpenFile(dirSector);
	directory->FetchFrom(dirFile);
	sector = directory->Find(nameBuffer);
	if (sector >= 0) {
		if(!directory->IsDirectory(nameBuffer)) {
			openFile = new OpenFile(sector);	// name was found in directory
			printf("File [%s(%d)] opened in directory [%d].\n",nameBuffer,sector,dirSector);
		} else {
			printf("File [%s] is a directory.\n",nameBuffer);
			delete directory;
			return NULL;
		}

		//		printf("File %s found at No.%d sector\n",name,sector);
	} else {
		printf("File [%s] not found.\n",nameBuffer);
	}
	delete directory;
	return openFile;				// return NULL if not found
}

bool FileSystem::RecurseRemove(char *name) {//just modified the DirectoryEntry

	char *finalName = new char[strlen(name)+1];
	char *temp = new char[strlen(name)+1];
	OpenFile* tempDirFile;//initially to be the current directory
	Directory* dir = new Directory(DirectorySector);
	int sector = DirectorySector;//initially to be root

	if(name[0] == '/') {//Absolute path
		strcpy(finalName, (char *)(name + 1));//remove the first '/'
		strcpy(name, finalName);
		tempDirFile = new OpenFile(DirectorySector);//change the ptr to root directory, parse start from root
	} else {
		strcpy(finalName, name);
		tempDirFile = currentDirectoryFile;//not absolute path, parse start from current directory
	}
	dir->FetchFrom(tempDirFile);//fetch the directory content from disk. This is the directory where the file is to be located.

	int i = 1;//counter for counting slashes.
	if((char)*strrchr(finalName, '/') == name[strlen(name) - 1]) {//name ends with slash(es).
		finalName[strlen(finalName) - 1] = '\0';//remove the last '/'
	}



	if(!strchr(finalName, '/')) {//No more slashes among the file path implicates that file is in root directory
		strcpy(name, finalName);
		if(tempDirFile != currentDirectoryFile && tempDirFile != rootDirectoryFile) {
			delete tempDirFile;
		}
		printf("File [%s] was found or should be created in [root(%d)]\n",name,sector);
		delete dir;
		delete temp;
		delete finalName;
		return sector;
	}

	while(strchr(finalName, '/')) {//If file is not in root then it must be in one of the root's child directory.
		//Note: assume there is no more than one slash joined together.
		strncpy(temp, finalName, strchr(finalName, '/') - finalName);//find the first directory in the path.
		temp[strchr(finalName, '/') - finalName] = '\0';

		strcpy(finalName, (char *)(strchr(finalName, '/') + 1));//cut the first directory from finalName.
		sector = dir->Find(temp);//see if the first directory exists (its father directory)

		if(sector < 0 || !dir->IsDirectory(temp)) {//if not
			if(tempDirFile != currentDirectoryFile && tempDirFile != rootDirectoryFile) {
				delete tempDirFile;
			}
			printf("File [%s] is not a directory or Directory [%s] does not exist.\n",temp,temp);
			delete dir;
			delete temp;
			delete finalName;
			return -1;
		}

		if(tempDirFile->GetFileDescriptor() != currentDirectoryFile->GetFileDescriptor() && tempDirFile->GetFileDescriptor() != rootDirectoryFile->GetFileDescriptor()) {//deallocate the buffer for dir
			delete tempDirFile;
		}
		//		dirHdr->FetchFrom(sector);
		tempDirFile = new OpenFile(sector);
		dir->FetchFrom(tempDirFile);//step into next directory and loop again.
	}
	//	if(!strcmp(finalName, temp)) {//is directory, return its father
	//		printf("Could not open a directory file for write for now...\n");
	//		return -1;
	//	}
	//	if(dir->Find(finalName) >= 0) {
	//		if(tempDirFile != currentDirectoryFile && tempDirFile != rootDirectoryFile) {
	//			delete tempDirFile;
	//		}
	//		strcpy(name, finalName);
	//		printf("File %s already exists.\n",finalName);
	//		return -1;
	//	}

	strcpy(name, finalName);
	printf("File [%s] was found or should be created in [%s(%d)]\n",name,temp,sector);
	if(tempDirFile->GetFileDescriptor() != currentDirectoryFile->GetFileDescriptor() && tempDirFile->GetFileDescriptor() != rootDirectoryFile->GetFileDescriptor()) {//deallocate the buffer for dir
		delete tempDirFile;
	}
	delete dir;
	delete temp;
	delete finalName;
	return sector;//return the directory sector where the file is to be created.
}
//----------------------------------------------------------------------
// FileSystem::Remove
// 	Delete a file from the file system.  This requires:
//	    Remove it from the directory
//	    Delete the space for its header
//	    Delete the space for its data blocks
//	    Write changes to directory, bitmap back to disk
//
//	Return TRUE if the file was deleted, FALSE if the file wasn't
//	in the file system.
//
//	"name" -- the text name of the file to be removed
//----------------------------------------------------------------------

bool
FileSystem::Remove(char *name)
{ 
	char* nameBuffer;
	Directory *directory;
	BitMap *freeMap;
	FileHeader *fileHdr;
	int sector, dirSector;

	nameBuffer = new char[strlen(name) + 1];
	strcpy(nameBuffer, name);
	dirSector = ParseDirectory(nameBuffer);
	OpenFile *dirFile = new OpenFile(dirSector);
	directory = new Directory(dirSector);
	directory->FetchFrom(dirFile);
	sector = directory->Find(nameBuffer);
	printf("Waitting for removing file [%s(%d)] from directory [%d].\n",nameBuffer,sector,dirSector);
	if (sector == -1) {
		delete directory;
		return FALSE;			 // file not found
	}
	fileHdr = new FileHeader;
	fileHdr->FetchFrom(sector);

	freeMap = new BitMap(NumSectors);
	freeMap->FetchFrom(freeMapFile);

	fileHdr->Deallocate(freeMap);// remove data blocks
	freeMap->Clear(sector);			// remove header block
	freeMap->WriteBack(freeMapFile);
	directory->Remove(nameBuffer);

//	freeMap->WriteBack(freeMapFile);		// flush to disk
	directory->WriteBack(dirFile);        // flush to disk

	delete fileHdr;
	delete dirFile;
	delete directory;
	delete freeMap;
	return TRUE;
} 

//----------------------------------------------------------------------
// FileSystem::List
// 	List all the files in the file system directory.
//----------------------------------------------------------------------

void
FileSystem::List()
{
	Directory *directory = new Directory(DirectorySector);

	directory->FetchFrom(directoryFile);
	directory->List();
	delete directory;
}

//----------------------------------------------------------------------
// FileSystem::Print
// 	Print everything about the file system:
//	  the contents of the bitmap
//	  the contents of the directory
//	  for each file in the directory,
//	      the contents of the file header
//	      the data in the file
//----------------------------------------------------------------------

void
FileSystem::Print()
{
	FileHeader *bitHdr = new FileHeader;
	FileHeader *dirHdr = new FileHeader;
	BitMap *freeMap = new BitMap(NumSectors);
	Directory *directory = new Directory(DirectorySector);

	printf("Bit map file header:\n");
	bitHdr->FetchFrom(FreeMapSector);
	bitHdr->Print();

	printf("Directory file header:\n");
	dirHdr->FetchFrom(DirectorySector);
	//	printf("\nDIR header begin!!!\n");
	dirHdr->Print();
	//	printf("\nDIR header done!!!\n");
	freeMap->FetchFrom(freeMapFile);
	//	printf("\nBITMAP content begin!!!\n");
	freeMap->Print();
	//	printf("\nBITMAP content done!!!\n");
	directory->FetchFrom(rootDirectoryFile);
	//	printf("\nDIR content begin!!!\n");
	directory->Print();
	//	printf("\nDIR content done!!!\n");

	delete bitHdr;
	delete dirHdr;
	delete freeMap;
	delete directory;
}
