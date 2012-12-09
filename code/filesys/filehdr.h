// filehdr.h 
//	Data structures for managing a disk file header.  
//
//	A file header describes where on disk to find the data in a file,
//	along with other information about the file (for instance, its
//	length, owner, etc.)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef FILEHDR_H
#define FILEHDR_H

#include "disk.h"
#include "time.h"//temperately use UNIX time.
#include "inode.h"

//Number of indexes in a header
#define NumDirect ((SectorSize - 4*sizeof(int) - 3*sizeof(int))/sizeof(int))
#define NumSingle (SectorSize/sizeof(int) - NumDirect - 1 - 1)// is 1
#define NumDouble (SectorSize/sizeof(int) - NumDirect - NumSingle - 1)// is 1
#define NumTriple (SectorSize/sizeof(int) - NumDirect - NumSingle - NumDouble)// is 1

#define MaxDirectSize NumDirect
#define MaxSingleSize (SectorSize/sizeof(int))
#define MaxDoubleSize (MaxSingleSize * SectorSize/sizeof(int))
#define MaxTripleSize (MaxDoubleSize * SectorSize/sizeof(int))
#define MaxFileSize 	((NumDirect + MaxSingleSize + MaxDoubleSize + MaxTripleSize) * SectorSize)//but nachos only support 128k address space

#define MaxDirectorySize 200
#define MaxFileNum NumSectors
/*
 * File types
 *
 * NOTE! These match bits 12..15 of stat.st_mode
 * (ie "(i_mode >> 12) & 15").
 */
#define DT_NORMAL       0
#define DT_FIFO         1
#define DT_DISKBITMAP   2
#define DT_DIR          3
#define DT_INODE				4
#define DT_BLK          6
#define DT_REG          8
#define DT_LNK          10
#define DT_SOCK         12
#define DT_WHT          14

#define FreeMapFileSize 	(NumSectors / BitsInByte)
#define NumDirEntries 		10
#define DirectoryFileSize 	(sizeof(DirectoryEntry) * NumDirEntries)

extern BitMap* currentFreeMap;
// The following class defines the Nachos "file header" (in UNIX terms,  
// the "i-node"), describing where on disk to find all of the data in the file.
// The file header is organized as a simple table of pointers to
// data blocks. 
//
// The file header data structure can be stored in memory or on disk.
// When it is on disk, it is stored in a single sector -- this means
// that we assume the size of this data structure to be the same
// as one disk sector.  Without indirect addressing, this
// limits the maximum file length to just under 4K bytes.
//
// There is no constructor; rather the file header can be initialized
// by allocating blocks for the file (if it is a new file), or by
// reading it from disk.

class FileHeader {
public:
	FileHeader();
	bool Allocate(BitMap *bitMap, int fileType, int parentSector);// Initialize a file header,
	//  including allocating space
	//  on disk for the file data
	bool Append(int bytesToBeAdded);
	void Deallocate(BitMap *bitMap);  		// De-allocate this file's
	//  data blocks

	void FetchFrom(int sectorNumber); 	// Initialize file header from disk
	void WriteBack(int sectorNumber); 	// Write modifications to file header
	//  back to disk

	int ByteToSector(int offset);	// Convert a byte offset into the file
	// to the disk sector containing
	// the byte
	int ByteToINodeSector(int offset);
	int FileLength();			// Return the length of the file
	int getFileType() { return fileType; }
	void IncFileLength(int bytesToAdd, int sectorsToAdd) { numBytes += bytesToAdd; numSectors += sectorsToAdd; }
	void DecFileLength(int bytesToDel, int sectorsToDel) {
		ASSERT(bytesToDel <= numBytes && sectorsToDel <= numSectors);
		numBytes -= bytesToDel;
		numSectors -= sectorsToDel;
	}
	// in bytes
	int FreeSpace();

	void Print();			// Print the contents of the file.

private:
	int numBytes;			// Number of bytes in the file
	int numSectors;			// Number of data sectors in the file
	int fileType;
	int parentSector;

	int dataSectors[NumDirect];		// Disk sector numbers for each data. Direct index.
	// If one of the three below is -1 that means there has no more data.
	int singleIndex;
	int doubleIndex;
	int tripleIndex;

	// The vars below would not be wrote back to disk
	//int currentFreeMap = 0;
};

#endif // FILEHDR_H
