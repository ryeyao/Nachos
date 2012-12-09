/*
 * inode.h
 *
 *  Created on: 2012-11-3
 *      Author: rye
 */

#ifndef INODE_H_
#define INODE_H_

#include "disk.h"
#include "bitmap.h"
#include "openfile.h"

class inode {
public:
	inode();//number of bytes in a sector. Pre-defined by SectorSize
	virtual ~inode();
	int Find(BitMap *freeMap, int virtualAddr);
	void Remove(int sector);
	void FetchFrom(int sector);  	// Init inode contents from disk
	void WriteBack(int sector);	// Write modifications to
																	// i_node contents back to disk
	bool Check(int virtualAddr);
	int Size() { return SectorSize/sizeof(int); }
private:
	int subNode[SectorSize/sizeof(int)];
	//Vars below will not be wrote back to disk

};
#endif /* INODE_H_ */
