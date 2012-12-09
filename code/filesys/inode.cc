/*
 * inode.cpp
 *
 *  Created on: 2012-11-3
 *      Author: rye
 */

#include "inode.h"
#include "filehdr.h"
#include "system.h"
#include "disk.h"
inode::inode() {
	// TODO Auto-generated constructor stub
	for(int i = 0;i < SectorSize/sizeof(int);i++) {
		subNode[i] = -1;
	}
}

inode::~inode() {
	// TODO Auto-generated destructor stub
}
int inode::Find(BitMap* freeMap, int virtualAddr) {
	int sinodeNum = virtualAddr;//data sector number
	//	for(int i = 0;i<SectorSize/BitsInByte;i++) {
	//		if(subNode[i] != -1) {
	//			subNode[i] = freeMap->Find();
	//			return subNode[i];
	//		}
	//	}
	if(!freeMap) {
//		printf("freeMap is null\n");
		return subNode[sinodeNum];
	}
	if(subNode[sinodeNum] == -1) {
		subNode[sinodeNum] = freeMap->Find();
//		printf("inode Find subNode is -1 not exist, allocate %d\n",subNode[sinodeNum]);
	}
//	printf("inode %d Find subNode is %d\n",sinodeNum,subNode[sinodeNum]);
	return subNode[sinodeNum];
}

void inode::Remove(int sector) {
	for(int i = 0;i<SectorSize/BitsInByte;i++) {
		if(subNode[i] == sector) {
			subNode[i] = -1;
			break;
		}
	}

}
bool inode::Check(int virtualAddr) {
	int sinodeNum = virtualAddr;
	if(subNode[sinodeNum] == -1) {
		return false;
	}
	return true;
}
//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------

void
inode::FetchFrom(int sector)
{

	synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------

void
inode::WriteBack(int sector)
{
	synchDisk->WriteSector(sector, (char *)this);
}
