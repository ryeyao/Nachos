/*
 * TS.cc
 *
 *  Created on: 2012-10-14
 *      Author: rye
 */

#include "ts.h"
#include "system.h"

Ts::Ts() {
	//do nothing
}

Ts::~Ts() {
	//do nothing
}

void Ts::PrintThreadInfo() {

	printf("Tid\tUid\tThread Name\tStatus\t\tPriority\tSlices\n");
	//printf("%d\t%d\t%s\t\tRUNNING\n",currentThread->getTid(),currentThread->getUid(),currentThread->getName());
	scheduler->Print();//print the content of readylist.
}



