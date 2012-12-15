// fakesocket.cc
//  Implement a fake socket for nachos
//
//  Created on: 2012-12-14
//      Author: rye

#include "fakesocket.h"
#include "system.h"
#include "utility.h"

FakeSocket::FakeSocket(NetworkAddress dest, FakeSocketType type) {

	outPktHdr.to = dest;
	outMailHdr.to = 0;
	outMailHdr.from = 1;
	mailList = new List();
	mailHdrList = new List();

}

FakeSocket::~FakeSocket() {

	delete mailList;
	delete mailHdrList;
}

int FakeSocket::Send(char* data) {

	ASSERT(data != NULL);
	DEBUG('n', "Sending data using FakeSocket::Send\n");

	int bytesSent = 0;
	outMailHdr.totalSlices = divRoundUp(strlen(data) + 1, MaxMailSize);
	outMailHdr.length = MaxMailSize;
	outPktHdr.totalSlices = outMailHdr.totalSlices;

	for (int i = 0; i < outMailHdr.totalSlices; i++) {

		outMailHdr.sliceIndex = i;

		if (i == outMailHdr.totalSlices - 1) {
			outMailHdr.length = (strlen(data) + 1) % MaxMailSize;
		}

		postOffice->Send(outPktHdr, outMailHdr, data + bytesSent);
		bytesSent += outMailHdr.length;
	}

	//ReceiveACK();
	return bytesSent;

}

int FakeSocket::Receive(char* into, int numBytes) {

	int mailCounts = divRoundUp(numBytes, MaxMailSize);
	int bytesRead = 0;
	char* buffer = NULL;	

	DEBUG('n', "Receiving data using FakeSocket::Receive\n");
	for(int i = 0 ; i < mailCounts; i++) {

		buffer = new char[MaxMailSize];
		postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
		//printf("\nBuffer is %s index %d\n", buffer, inMailHdr.sliceIndex);
		mailList->SortedInsert((void *)buffer, inMailHdr.sliceIndex);
		mailHdrList->SortedInsert((void *)&inMailHdr, inMailHdr.sliceIndex);
		bytesRead += inMailHdr.length;
	}

	DEBUG('n', "Data received has %d mails or %d bytes in total\n", inMailHdr.totalSlices, bytesRead);
	int length = 0;
	while (!mailList->IsEmpty()) {
		inMailHdr = *((MailHeader *)mailHdrList->Remove());
		buffer =(char *)mailList->Remove();
		bcopy(buffer, into + length, inMailHdr.length );
		length += inMailHdr.length;
		delete[] buffer;
	}
	
	//SendACK();
	return bytesRead;
}

void FakeSocket::SendACK() {
	
	DEBUG('n', "Sending ACK=%c from %d to %d\n", ACK, outPktHdr.from, outPktHdr.to);
	outPktHdr.to = inPktHdr.from;
	outPktHdr.totalSlices = 1;
	outPktHdr.sliceIndex = 0;
	outMailHdr.to = inMailHdr.from;
	outMailHdr.length = 1;
	outMailHdr.totalSlices = 1;
	outMailHdr.sliceIndex = 0;
	char ack = ACK;
	postOffice->Send(outPktHdr, outMailHdr, &ack);
	fflush(stdout);
}

void FakeSocket::ReceiveACK() {
	
	char ack;
	postOffice->Receive(1, &inPktHdr, &inMailHdr, &ack);

	DEBUG('n', "Received ACK=%c from %d to %d\n", ack, inPktHdr.from, inPktHdr.to);
	fflush(stdout);
}
