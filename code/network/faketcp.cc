// faketcp.cc
//  Implement a fake TCP protocol for nachos.
//  Note that we do not have a real TCP header structure.
//
//  Created on: 2012-12-14
//      Author: rye

#include "system.h"
#include "faketcp.h"
#include "utility.h"

FakeTCP::FakeTCP(NetworkAddress dest) {

	outPktHdr.to = dest;
	outMailHdr.to = 0;
	outMailHdr.from = 1;
	mailList = new List();
	mailHdrList = new List();
}

FakeTCP::~FakeTCP() {

	if (dataSent) {
		delete[] dataSent;
	}
	delete mailHdrList;
	delete mailList;
}

int FakeTCP::Send(char* data) {

	return SendAll(data);
}

// Send data of index *index*
void FakeTCP::SendOne(int index) {


		outMailHdr.sliceIndex = index;
		outMailHdr.length = dataLengths[index];
		postOffice->Send(outPktHdr, outMailHdr, dataSent[index]);
}
int FakeTCP::SendAll(char* data) {

	int bytesSent = 0;
	outMailHdr.totalSlices = divRoundUp(strlen(data) + 1, MaxMailSize);
	outMailHdr.length = MaxMailSize;
	outPktHdr.totalSlices = outMailHdr.totalSlices;
	char* dataSlice;
	dataSent = new char*[outMailHdr.totalSlices];
	dataLengths = new int[outMailHdr.totalSlices];

	// divide data into packets and send
	for (int i = 0; i < outMailHdr.totalSlices; i++) {

		outMailHdr.sliceIndex = i;

		if (i == outMailHdr.totalSlices - 1) {
			outMailHdr.length = (strlen(data) + 1) % MaxMailSize;
		}

		// add data into dataSent list
		dataSlice = new char[MaxMailSize];
		strncpy(dataSlice, data + bytesSent, outMailHdr.length);
		dataSent[i] = dataSlice;
		dataLengths[i] = outMailHdr.length;

		// Send data of index *i* to remote machine. 
		// Check if the data had been received.
		int timeout = 0;
		SendOne(i);
		while (timeout++ < MAX_TRY && !ReceiveACK(i)) {

			DEBUG('n', "No response! Send again...\n");
			SendOne(i);
		}
		bytesSent += outMailHdr.length;
		if (timeout >= MAX_TRY) {
			delete[] dataSlice;
			return bytesSent;
		}
	}
	delete[] dataSlice;
	return bytesSent;
}

// FakeTCP::ReceiveACK
//  Simply wait for response
bool FakeTCP::ReceiveACK(int index) {

	char* buffer = NULL;	

	buffer = new char[MaxMailSize];
	postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
    char* ack = new char[inMailHdr.length];
	strncpy(ack, buffer, inMailHdr.length);
	//printf("\nBuffer is %s index %d\n", buffer, inMailHdr.sliceIndex);
	if (atoi(ack) != index) {
		DEBUG('n', "FakeTCP ReceiveACK ------wrong ack = %d.\n", atoi(ack));
		delete[] ack;
		delete[] buffer;
		return false;
	}
	DEBUG('n', "FakeTCP ReceiveACK ------succeed.\n");
	delete[] ack;
	delete[] buffer;
	return true;

}

// FakeTCP::Receive
// Retrive data
int FakeTCP::Receive(char* into) {

	int bytesRead = 0;
	char* buffer = NULL;	

	DEBUG('n', "Receiving data using FakeSocket::Receive\n");

	int timeout = 0;
	while(true) {
		buffer = new char[MaxMailSize];
		postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
		//printf("\nBuffer is %s index %d\n", buffer, inMailHdr.sliceIndex);
		if (inMailHdr.sliceIndex == 0) {
			lastIndex = 0;
		} 
	
		SendACK(inMailHdr.sliceIndex);
		if (lastIndex != 0 && lastIndex + 1 == inMailHdr.sliceIndex) {
			mailList->SortedInsert((void *)buffer, inMailHdr.sliceIndex);
			mailHdrList->SortedInsert((void *)&inMailHdr, inMailHdr.sliceIndex);
			bytesRead += inMailHdr.length;
		} else if (lastIndex != 0) {
			delete[] buffer;
		}
		lastIndex = inMailHdr.sliceIndex;
		timeout++;
		if (lastIndex == inMailHdr.totalSlices || timeout >= MAX_TRY * inMailHdr.totalSlices) 
		{
			DEBUG('n', "FakeTCP::Receive all data received successfully.\n");
			break;
		}
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

	return bytesRead;

}

bool FakeTCP::SendACK(int index) {

	outPktHdr.to = inPktHdr.from;
	outPktHdr.totalSlices = 1;
	outPktHdr.sliceIndex = 0;
	outMailHdr.to = inMailHdr.from;
	outMailHdr.length = 5;
	outMailHdr.totalSlices = 1;
	outMailHdr.sliceIndex = 0;
	char ack[5];
	sprintf(ack, "%5d", index);

	DEBUG('n', "Sending ACK=%s(atoi(ACK = %d)) from %d to %d\n", ack, atoi(ack), outPktHdr.from, outPktHdr.to);
	postOffice->Send(outPktHdr, outMailHdr, ack);
	fflush(stdout);
	return true;

}
