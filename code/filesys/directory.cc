// directory.cc 
//	Routines to manage a directory of file names.
//
//	The directory is a table of fixed length entries; each
//	entry represents a single file, and contains the file name,
//	and the location of the file header on disk.  The fixed size
//	of each directory entry means that we have the restriction
//	of a fixed maximum size for file names.
//
//	The constructor initializes an empty directory of a certain size;
//	we use ReadFrom/WriteBack to fetch the contents of the directory
//	from disk, and to write back any modifications back to disk.
//
//	Also, this implementation has the restriction that the size
//	of the directory cannot expand.  In other words, once all the
//	entries in the directory are used, no more files can be created.
//	Fixing this is one of the parts to the assignment.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "filesys.h"
#include "filehdr.h"
#include "directory.h"
#include "string.h"

//----------------------------------------------------------------------
// Directory::Directory
// 	Initialize a directory; initially, the directory is completely
//	empty.  If the disk is being formatted, an empty directory
//	is all we need, but otherwise, we need to call FetchFrom in order
//	to initialize it from disk.
//
//	"size" is the number of entries in the directory
//----------------------------------------------------------------------

Directory::Directory(int selfSector)
{
	//	table = new DirectoryEntry[parent];
	//	tableSize = parent;
	table = NULL;
	tableSize = 0;
	this->selfSector = selfSector;
	//	for (int i = 0; i < tableSize; i++)
	//		table[i].inUse = FALSE;
}

//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------

Directory::~Directory()
{ 
	if(tableSize > 0) {
		delete [] table;
	}
} 

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------

void
Directory::FetchFrom(OpenFile *file)
{
	selfSector = file->GetFileDescriptor();
	if(tableSize != 0) {//current directory is not empty
		delete[] table;
		tableSize = 0;
	} else {
		table = NULL;
	}
	tableSize = file->Length()/sizeof(DirectoryEntry);//new directory file size

	if(tableSize > 0) {//the directory file to be fetched is not empty
		table = new DirectoryEntry[tableSize];
		(void) file->ReadAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
	} else {
		printf("FetchFrom empty directory(file length is %d), do nothing\n",file->Length());
	}
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------

void
Directory::WriteBack(OpenFile *file)
{
	int num = 0;
	if(tableSize != 0) {
		num = file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
		printf("Directory file(%d bytes) is written back to disk\n",num);
	}
	printf("Directory file(%d bytes) is written back to disk\n",num);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::FindIndex(char *name)
{
	if(tableSize == 0) {
		return -1;//new directory
	}
	for (int i = 0; i < tableSize; i++) {
		//if (table[i].inUse && !strncmp(table[i].name, name, FileNameMaxLen))
		if (table[i].inUse && !strncmp(table[i].name, name, FileNameMaxLen))
			return i;
	}
	return -1;		// name not in directory
}

//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't 
//	in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::Find(char *name)
{
	int i = FindIndex(name);

	if (i != -1) {
		//Assume that when Find is called, the file (if exist) would be modified/accessed.
		time_t lt;
		lt = time(NULL);
		char *ts = ctime(&lt);
		int ts_len = strlen(ts) + 1;
		strncpy(table[i].lastAccessTime, ts, ts_len);
		strncpy(table[i].lastModifyTime, ts, ts_len);
		return table[i].sector;
	}
	return -1;
}

bool Directory::IsDirectory(char *name) {

	int i = FindIndex(name);

	if(i != -1) {
		if(table[i].fileType == DT_DIR) {
			return true;
		}
	}
	return false;

}
//----------------------------------------------------------------------
// Directory::Add
// 	Add a file into the directory.  Return TRUE if successful;
//	return FALSE if the file name is already in the directory, or if
//	the directory is completely full, and has no more space for
//	additional file names.
//
//	"name" -- the name of the file being added
//	"newSector" -- the disk sector containing the added file's header
//----------------------------------------------------------------------

bool
Directory::Add(char *name, int newSector, int fileType)
{	DirectoryEntry *de = NULL;
	if (FindIndex(name) != -1) {//file exists or new directory
		if(tableSize != 0) {//file exists
			return FALSE;
		}
	}
	if(tableSize + 1 > MaxDirectorySize) {
		printf("No more spaces in this directory.\n");
		return FALSE;
	}
	de = new DirectoryEntry[tableSize + 1];
	memcpy(de, table, sizeof(DirectoryEntry) * tableSize);
	delete[] table;//delete old table
	table = de;
	table[tableSize].inUse = TRUE;
	strncpy(table[tableSize].name, name, FileNameMaxLen);
	table[tableSize].sector = newSector;

	//initialize some of the file's attributes
	table[tableSize].fileType = fileType;
	//			printf("filehdr.cc:Allocate\n");
	time_t lt;
	lt = time(NULL);
	char *ts = ctime(&lt);
	int ts_len = strlen(ts)+1;
	strncpy(table[tableSize].createTime, ts, ts_len);
	strncpy(table[tableSize].lastAccessTime, ts, ts_len);
	strncpy(table[tableSize].lastModifyTime, ts, ts_len);
	tableSize++;
	return TRUE;	// no space.  Fix when we have extensible files.
}

//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory. 
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------

bool
Directory::Remove(char *name)
{	int i = FindIndex(name);
	if (tableSize <= 0 || i == -1) {
		return FALSE; 		// name not in directory
	}
	FileHeader *selfHdr = new FileHeader;
	BitMap *currentFreeMap = new BitMap(NumSectors);
	currentFreeMap->FetchFrom(currentFreeMapFile);
	selfHdr->FetchFrom(selfSector);
	if (tableSize == 1) {// only one file int it.
		tableSize = 0;
		selfHdr->Deallocate(currentFreeMap);//old directory file on disk is abandoned
		ASSERT(selfHdr->FileLength() == 0);
		delete[] table;
		delete currentFreeMap;
		delete selfHdr;
		return TRUE;
	}
	DirectoryEntry *de = new DirectoryEntry[tableSize - 1];
	memcpy(de, table, i*sizeof(DirectoryEntry));
	memcpy(de + i, table + i + 1, (tableSize - i - 1));
	tableSize--;
	delete[] table;
	table = de;
	int lastSector = divRoundDown(selfHdr->FileLength(), SectorSize);
	if (divRoundDown(selfHdr->FileLength() - sizeof(DirectoryEntry), SectorSize) < lastSector) {
		ASSERT(currentFreeMap->Test(lastSector));  // ought to be marked!
		currentFreeMap->Clear(lastSector);

		printf("Clear the last sector of directory file\n");
	}
	delete currentFreeMap;
	delete selfHdr;
	return TRUE;
}

//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory. 
//----------------------------------------------------------------------

void
Directory::List()
{
	for (int i = 0; i < tableSize; i++)
		if (table[i].inUse)
			printf("%s\n", table[i].name);
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------

void
Directory::Print()
{ 
	FileHeader *hdr = new FileHeader;

	printf("Directory contents:\n");
	for (int i = 0; i < tableSize; i++)
		if (table[i].inUse) {
			printf("Name: %s, Sector: %d\n", table[i].name, table[i].sector);
			hdr->FetchFrom(table[i].sector);
			hdr->Print();
		}
	printf("\n");
	delete hdr;
}
