// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "bitmap.h"
#include "system.h"
#include "filehdr.h"
#include "directory.h"

FileHeader::FileHeader() {
	numBytes = 0;			// Number of bytes in the file
	numSectors = 0;			// Number of data sectors in the file
	fileType = -1;
	parentSector = -1;
	for (int i = 0; i < NumDirect; i++) {
		dataSectors[i] = -1;
	}		// Disk sector numbers for each data. Direct index.
	// If one of the three below is -1 that means there has no more data.
	singleIndex = -1;
	doubleIndex = -1;
	tripleIndex = -1;
}
//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------
/**
 * Modified by Rye
 * Allocate blocks for header only. No file size specified.
 * A file has at least a number of @NumDirect(has been initially set) blocks to use.
 */
//BitMap *currentFreeMap = NULL;
bool
FileHeader::Allocate(BitMap *freeMap, int fileType, int parentSector)
{ 
	this->fileType = fileType;
	this->parentSector = parentSector;

	switch(fileType) {
	case DT_DIR :
		//		numBytes = DirectoryFileSize;
		//		numSectors = divRoundUp(DirectoryFileSize, SectorSize);
		numBytes = 0;
		numSectors = 0;
		//		printf("DIR:\n");
		break;
	case DT_DISKBITMAP :
		numBytes = FreeMapFileSize;
		numSectors = divRoundUp(FreeMapFileSize, SectorSize);
		//		printf("BITMAP:\n");
		break;
	case DT_NORMAL :
		numBytes = 0;
		numSectors = 0;
		break;
	case DT_INODE :
		numBytes = SectorSize;
		numSectors = NumDirect;
		break;
	default :
		numBytes = SectorSize;
		numSectors = NumDirect;
		break;
	}
	//	numBytes = fileType;
	//	numSectors  = divRoundUp(fileType, SectorSize);
	//===========Actually start allocate===========
	//	currentFreeMap = freeMap;
	//Initialize multiple index to -1.
	singleIndex = -1;
	doubleIndex = -1;
	tripleIndex = -1;

	//	printf("freeMap->NumClear is %d, numSectors is %d\n", freeMap->NumClear(), numSectors);
	if (freeMap->NumClear() < numSectors)
		return FALSE;		// not enough space //for file's header
	//if((fileType == DT_DIR) || (fileType == DT_DISKBITMAP)) {
	if(fileType == DT_DISKBITMAP) {
		for (int i = 0; i < NumDirect; i++) {
			if(i == 0) {
				dataSectors[i] = freeMap->Find();
			} else {
				dataSectors[i] = -1;
			}
			//		dataSectors[i] = -1;
			//		printf("Allocated %d\n",dataSectors[i]);
		}
	} else if(fileType == DT_NORMAL || fileType == DT_DIR) {
		for (int i = 0; i < NumDirect; i++) {
			dataSectors[i] = -1;
		}
	}
	return TRUE;
}

bool FileHeader::Append(int bytesToAdd) {
	int sectorsToAdd = divRoundUp(bytesToAdd,SectorSize);
	int sSector = -1, dSector = -1;
	//	bool checkS = false, checkD = false, checkT = false;
	inode *sinode = new inode();
	inode *dinode = new inode();
	inode *tinode = new inode();
	BitMap *currentFreeMap = new BitMap(NumSectors);
	currentFreeMap->FetchFrom(currentFreeMapFile);
	if(sectorsToAdd > currentFreeMap->NumClear()) {
		printf("No more spaces in disk.\n");
		return false;
	}
	for(int i = 1; i <= sectorsToAdd; i++) {
		int vAddr = (numSectors + i) - MaxDirectSize - 1;
		if(vAddr + MaxDirectSize < NumDirect) {//Direct index
			//			ASSERT(dataSectors[vAddr + MaxDirectSize] == -1);
			if(dataSectors[vAddr + MaxDirectSize] != -1) {//no more than one sector
				continue;
			}
			dataSectors[vAddr + MaxDirectSize] = currentFreeMap->Find();
		} else if(vAddr < MaxSingleSize) {//next is single
			//			printf("Append: single\n");
			if(singleIndex < 0) {
				singleIndex = currentFreeMap->Find();
			} else {
				sinode->FetchFrom(singleIndex);
			}
			//			ASSERT(!sinode->Check(vAddr));
			if(sinode->Check(vAddr)) {
				continue;
			}
			sinode->Find(currentFreeMap, vAddr);
			sinode->WriteBack(singleIndex);
		} else if(vAddr < MaxSingleSize + MaxDoubleSize) {//next is double
			vAddr -= MaxSingleSize;
			//			printf("Append: double\n");
			//			ASSERT(dinode->Check(vAddr/MaxSingleSize));
			if(doubleIndex < 0) {
				doubleIndex = currentFreeMap->Find();
			} else {
				dinode->FetchFrom(doubleIndex);
			}
			if(!dinode->Check(vAddr/MaxSingleSize)) {
				sinode = new inode();
				sSector = dinode->Find(currentFreeMap,vAddr/MaxSingleSize);

			} else {
				sSector = dinode->Find(currentFreeMap,vAddr/MaxSingleSize);
				sinode->FetchFrom(sSector);
			}
			//			ASSERT(!sinode->Check(vAddr%MaxDoubleSize));
			if(sinode->Check(vAddr%MaxSingleSize)) {
				continue;
			}
			sinode->Find(currentFreeMap,vAddr%MaxSingleSize);

			sinode->WriteBack(sSector);
			dinode->WriteBack(doubleIndex);

		} else if(vAddr < MaxSingleSize + MaxDoubleSize + MaxTripleSize) {//next is triple
			vAddr -= MaxDoubleSize;
			//			printf("Append: triple\n");
			if(tripleIndex < 0) {
				tripleIndex = currentFreeMap->Find();
			} else {
				tinode->FetchFrom(tripleIndex);
			}

			if(!tinode->Check(vAddr/MaxDoubleSize)) {
				dinode = new inode();
				dSector = tinode->Find(currentFreeMap,vAddr/MaxDoubleSize);
			} else {
				dSector = tinode->Find(currentFreeMap,vAddr/MaxDoubleSize);
				dinode->FetchFrom(dSector);
			}
			if(!dinode->Check((vAddr/MaxSingleSize)%MaxSingleSize)) {
				sinode = new inode();
				sSector = dinode->Find(currentFreeMap,(vAddr/MaxSingleSize)%MaxSingleSize);
			} else {
				sSector = dinode->Find(currentFreeMap,(vAddr/MaxSingleSize)%MaxSingleSize);
				sinode->FetchFrom(sSector);
			}
			//			ASSERT(!sinode->Check(vAddr%MaxSingleSize));
			if(sinode->Check(vAddr%MaxSingleSize)) {
				continue;
			}
			sinode->Find(currentFreeMap,vAddr%MaxSingleSize);

			sinode->WriteBack(sSector);
			dinode->WriteBack(dSector);
			tinode->WriteBack(tripleIndex);
		}
	}
	//	printf("numBytes is %d, numSectors is %d\n",numBytes,numSectors);
	ASSERT(divRoundUp(numBytes,SectorSize) == numSectors);
	currentFreeMap->WriteBack(currentFreeMapFile);
	delete currentFreeMap;
	delete sinode;
	delete dinode;
	delete tinode;
	return true;
}
//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(BitMap *freeMap)
{
	for (int i = 0; i < numSectors; i++) {
		//printf("Test %d \n",ByteToSector(i*SectorSize));
		ASSERT(freeMap->Test((int) ByteToSector(i*SectorSize)));  // ought to be marked!
		freeMap->Clear((int) ByteToSector(i*SectorSize));
		if(i < NumDirect) {
			dataSectors[i] = -1;
		}
	}
	singleIndex = -1;
	doubleIndex = -1;
	tripleIndex = -1;
	numSectors = 0;
	numBytes = 0;
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
	/**
	 * Added by Rye
	 * Assume that when a FileHeader is fetched from disk, that means this file is to be visited.
	 */
	//	printf("filehdr.cc:FetchFrom\n");
	synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
	/**
	 * Added by Rye
	 */
	// Assume that when WriteBack is called, that means this file has been modified.
	//	printf("filehdr.cc:WriteBack\n");
	synchDisk->WriteSector(sector, (char *)this);
}



//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
	int virtualAddr = offset/SectorSize;
	int dataSector = -1, sSector = -1, dSector = -1;
	inode *sinode = new inode();
	inode *dinode = new inode();
	inode *tinode = new inode();

	if(virtualAddr < MaxDirectSize) {//direct
		//		printf("ByteToSector: Direct index %d\n",virtualAddr);
		dataSector = dataSectors[virtualAddr];
	} else if(virtualAddr < MaxDirectSize + MaxSingleSize) {//Single index //Assume that there is enough space
		virtualAddr -= MaxDirectSize;
		//		printf("ByteToSector: Single index is %d, virtualAddr is %d*********\n",singleIndex,virtualAddr);
		ASSERT(singleIndex >= 0);
		sinode->FetchFrom(singleIndex);
		//		printf("Prepare to check %d\n",virtualAddr);
		ASSERT(sinode->Check(virtualAddr));
		dataSector = sinode->Find(NULL, virtualAddr);
		//		printf("Let me see what was wrong\n");
	} else if(virtualAddr < MaxDirectSize + MaxSingleSize + MaxDoubleSize) {//Double index
		virtualAddr -= MaxDirectSize + MaxSingleSize;
		//		printf("ByteToSector: Double index\n");
		ASSERT(doubleIndex >= 0 && singleIndex >=0);
		dinode->FetchFrom(doubleIndex);
		ASSERT(dinode->Check(virtualAddr/MaxSingleSize));
		sSector = dinode->Find(NULL, virtualAddr/MaxSingleSize);
		sinode->FetchFrom(sSector);
		ASSERT(sinode->Check(virtualAddr%MaxSingleSize));
		dataSector = sinode->Find(NULL, virtualAddr%MaxSingleSize);

	} else if(virtualAddr < MaxDirectSize + MaxSingleSize + MaxDoubleSize + MaxTripleSize) {//Triple index
		virtualAddr -= MaxDirectSize + MaxSingleSize + MaxDoubleSize;
		ASSERT(tripleIndex >= 0 && doubleIndex >= 0 && singleIndex >= 0);
		tinode->FetchFrom(tripleIndex);
		//		printf("ByteToSector: Triple index\n");
		ASSERT(tinode->Check(virtualAddr/MaxDoubleSize));
		dSector = tinode->Find(NULL, virtualAddr/MaxDoubleSize);
		dinode->FetchFrom(dSector);
		ASSERT(dinode->Check((virtualAddr%MaxDoubleSize)/MaxSingleSize));
		sSector = dinode->Find(NULL, (virtualAddr%MaxDoubleSize)/MaxSingleSize);
		sinode->FetchFrom(dSector);
		ASSERT(sinode->Check(virtualAddr%MaxSingleSize));
		dataSector = sinode->Find(NULL, virtualAddr%MaxSingleSize);

	}
	delete sinode;
	delete dinode;
	delete tinode;
	//		printf("fucking dataSector to be returned = %d\n",dataSector);
	return dataSector;
	//	}
	//	delete sinode;
	//	delete dinode;
	//	delete tinode;
	//	return -1;
}

int FileHeader::ByteToINodeSector(int offset) {
	int virtualAddr = offset/SectorSize;
	int dataSector = -1, sSector = -1, dSector = -1;
	//	bool checkS = false, checkD = false, checkT = false;
	inode *sinode = new inode();
	inode *dinode = new inode();
	inode *tinode = new inode();

	////	printf("FileHeader ByteToINodeSector where am I??\n");
	//	if(!((BitMap *)currentFreeMap) || ((BitMap *)currentFreeMap)->NumClear()) {
	////		printf("FileHeader ByteToINodeSector where am I-2??\n");
	//	ASSERT(!currentFreeMap);
	//	printf("virtualAddr is %d\n",virtualAddr);
	if(virtualAddr < MaxDirectSize) {//direct
		//		printf("Direct index %d, dataSector is %d\n",virtualAddr,dataSectors[virtualAddr]);
		dataSector = dataSectors[virtualAddr];
		delete sinode;
		delete dinode;
		delete tinode;
		return dataSector;

	} else {
		BitMap *currentFreeMap = new BitMap(NumSectors);
		currentFreeMap->FetchFrom(currentFreeMapFile);
		if(virtualAddr < MaxDirectSize + MaxSingleSize) {//Single index //Assume that there is enough space
			virtualAddr -= MaxDirectSize;
			//			printf("Single index is %d, singleSize is %d,vAddr is %d*********\n",singleIndex,MaxDirectSize + MaxSingleSize,virtualAddr);
			if(singleIndex < 0) {//no single index yet
				//				printf("Single index not exist\n");
				singleIndex = ((BitMap *)currentFreeMap)->Find();//allocate a sector for single inode
			} else {
				//				printf("SingleIndex already exist,fetch from %d\n",singleIndex);
				sinode->FetchFrom(singleIndex);
			}
			//			checkS = sinode->Check(virtualAddr);

			dataSector = sinode->Find(((BitMap *)currentFreeMap), virtualAddr);
			//			printf("hey im here\n");
			//			if(!checkS) {
			sinode->WriteBack(singleIndex);//write single inode back to disk
			//			}

		} else if(virtualAddr < MaxDirectSize + MaxSingleSize + MaxDoubleSize) {//Double index
			virtualAddr -= MaxDirectSize + MaxSingleSize;
			//			printf("Double index\n");
			if(doubleIndex < 0) {//no double index yet
				doubleIndex = ((BitMap *)currentFreeMap)->Find();//allocate a sector for double inode
			} else {
				dinode->FetchFrom(doubleIndex);
			}
			//			checkD = dinode->Check(virtualAddr/MaxSingleSize);
			sSector = dinode->Find(((BitMap *)currentFreeMap), virtualAddr/MaxSingleSize);
			sinode->FetchFrom(sSector);
			//			checkS = sinode->Check(virtualAddr%MaxSingleSize);

			dataSector = sinode->Find(((BitMap *)currentFreeMap), virtualAddr%MaxSingleSize);
			//			if(!checkS || !checkD) {
			dinode->WriteBack(doubleIndex);//write double inode back to disk
			sinode->WriteBack(sSector);
			//			}
		} else if(virtualAddr < MaxDirectSize + MaxSingleSize + MaxDoubleSize + MaxTripleSize) {//Triple index
			virtualAddr -= MaxDirectSize + MaxSingleSize + MaxDoubleSize;
			if(tripleIndex < 0) {//no triple index yet
				tripleIndex = ((BitMap *)currentFreeMap)->Find();//allocate a sector for triple inode
			} else {
				tinode->FetchFrom(tripleIndex);
			}
			//			printf("Triple index\n");
			//			checkT = tinode->Check(virtualAddr/MaxDoubleSize);
			dSector = tinode->Find(((BitMap *)currentFreeMap), virtualAddr/MaxDoubleSize);
			dinode->FetchFrom(dSector);
			//			checkD = dinode->Check((virtualAddr%MaxDoubleSize)/MaxSingleSize);
			sSector = dinode->Find(((BitMap *)currentFreeMap), (virtualAddr%MaxDoubleSize)/MaxSingleSize);
			sinode->FetchFrom(dSector);
			//			checkS = sinode->Check((virtualAddr%MaxDoubleSize)%MaxSingleSize);

			dataSector = sinode->Find(((BitMap *)currentFreeMap), virtualAddr);

			//			if(!checkS || !checkD || !checkT) {
			tinode->WriteBack(tripleIndex);
			dinode->WriteBack(dSector);
			sinode->WriteBack(sSector);
			//			}
		}
		currentFreeMap->WriteBack(currentFreeMapFile);
		delete currentFreeMap;
	}
	delete sinode;
	delete dinode;
	delete tinode;
	//	printf("fucking dataSector to be returned = %d\n",dataSector);
	return dataSector;
	//	}
	//	delete sinode;
	//	delete dinode;
	//	delete tinode;
	//	return -1;
}
//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
	return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
	int i, j, k;
	char *data = new char[SectorSize];

	printf("FileHeader contents.  File size: %d. File sectors: %d. File blocks:\n", numBytes, numSectors);
	for (i = 0; i < numSectors; i++)
		printf("%d ", ByteToSector(i));
	//	printf("File create time: %s\n",createTime);
	//	printf("File last visited time: %s\n",lastAccessTime);
	//	printf("File last modified time: %s\n",lastChangeTime);
	printf("\nFile contents:\n");
	for (i = k = 0; i < numSectors; i++) {
		synchDisk->ReadSector(dataSectors[i], data);
		for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
			if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
				printf("%c", data[j]);
			else
				printf("\\%x", (unsigned char)data[j]);
		}
		printf("\n");
		//		printf("\n***********i = %d, k = %d, numSectors = %d, SectorSize = %d***********\n", i,k,numSectors,SectorSize);
	}
	//	printf("Delete Successfully????\n");
	delete [] data;
	//	printf("Delete Successfully????\n");
}


/**
 * 1. Server crash
 *  solution: A backup server
 * 2. Privacy information
 *  solution: Build firewall....
 *  3.
 */
