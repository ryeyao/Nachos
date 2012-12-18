// faketcp.cc
//  Implement a fake TCP protocol for nachos.
//  Note that we do not have a real TCP header structure.
//
//  Created on: 2012-12-14
//      Author: rye

#include "system.h"
#include "faketcp.h"
#include "utility.h"

static void DebugPrintHeader(PacketHeader pktHdr, MailHeader mailHdr) {

	printf("From (%d, %d) to (%d, %d) bytes %d \n", pktHdr.from, mailHdr.from, pktHdr.to, mailHdr.to, mailHdr.length);

}

FakeTCP::FakeTCP(NetworkAddress dest) {

	destination = dest;
	lostRate = 0.0;
	totalLost = 0;
	totalReceive = 0;
}

FakeTCP::~FakeTCP() {

}

int FakeTCP::Send(char* data) {

	return SendAll(data);
}

// Send data of index *index*
void FakeTCP::SendOne(PacketHeader outPktHdr, MailHeader outMailHdr, char* data) {

	outMailHdr.to = 0;
	outMailHdr.from = 1;
	DEBUG('n', "Debug: SendOne data is %s\n", data);
	postOffice->Send(outPktHdr, outMailHdr, data);
	fflush(stdout);
}

int FakeTCP::SendAll(char* data) {

	int bytesSent = 0, i;
	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	// send data from local mail box #1 to remote mail box #0
	outMailHdr.to = 0;  
	outMailHdr.from = 1; 
	outMailHdr.totalSlices = divRoundUp(strlen(data) + 1, MaxMailSize);
	outMailHdr.length = MaxMailSize;

	outPktHdr.to = destination;
	outPktHdr.from = -1;
	outPktHdr.totalSlices = outMailHdr.totalSlices;

	DEBUG('n', "\n======>\nData len is %d, should be divided into %d slices\n", strlen(data) + 1, outMailHdr.totalSlices);
	// divide data into packets and send
	int length = 0;
	for (i = 0; i < outMailHdr.totalSlices; i++) {

		outMailHdr.sliceIndex = i;

		if (i == outMailHdr.totalSlices - 1) {
			outMailHdr.length = (strlen(data) + 1) % MaxMailSize;
		}

		// Send data to remote machine. 
		// Check if the data had been received.
		int timeout = 0;
		SendOne(outPktHdr, outMailHdr, data + length);
		length += outMailHdr.length;
		bool isSentRight = ReceiveACK(i);
		while (timeout++ < MAX_TRY && !isSentRight) {

			DEBUG('n', "FakeTCP::SendAll Packet may be lost, send again(%d)...\n", timeout);
			SendOne(outPktHdr, outMailHdr, data + length);
			isSentRight = ReceiveACK(i);
		}
		bytesSent += outMailHdr.length;
		if (timeout >= MAX_TRY) {
			DEBUG('n', "FakeTCP::SendAll No response!\n");
			break;
		}
	}
	return bytesSent;
}

// FakeTCP::ReceiveACK
//  Simply wait for response
bool FakeTCP::ReceiveACK(int index) {

	char* buffer = NULL;	
	PacketHeader inPktHdr;
	MailHeader inMailHdr;

	buffer = new char[MaxMailSize];
	postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);
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
	PacketHeader* inPktHdr;
	MailHeader* inMailHdr;
	List* mailList = new List();
	List* mailHdrList = new List();

	DEBUG('n', "Retrive data using FakeSocket::Receive\n");

	int timeout = 0;
	lastIndex = -1;
	while(true) {
		buffer = new char[MaxMailSize];
		inPktHdr = new PacketHeader();
		inMailHdr = new MailHeader();

		postOffice->Receive(0, inPktHdr, inMailHdr, buffer);
		DEBUG('n', "Debug: buffer is %s, len is %d, buffer hdr.length is %d\n", buffer, strlen(buffer), inMailHdr->length);
	
		if (lastIndex + 1 == inMailHdr->sliceIndex) {
			totalReceive++;
			DEBUG('n', "Got the right data packet. Insert #%d hdr(len = %d)\n", inMailHdr->sliceIndex, inMailHdr->length);

			mailList->SortedInsert((void *)buffer, inMailHdr->sliceIndex);
			mailHdrList->SortedInsert(inMailHdr, inMailHdr->sliceIndex);
			bytesRead += inMailHdr->length;
		} else {

			totalLost++;
			lostRate = totalLost / (totalLost + totalReceive);
			DEBUG('n', "Get the wrong data packet.(%.2f\% lost)\n", lostRate);
			SendACK(inPktHdr, inMailHdr);
			delete[] buffer;

			if (timeout++ >= MAX_TRY * inMailHdr->totalSlices) {
				DEBUG('n', "FakeTCP::Receive: receiving time out.\n");
				break;
			}
			continue;
		}

		SendACK(inPktHdr, inMailHdr);

		if (inMailHdr->sliceIndex == inMailHdr->totalSlices - 1 )
		{
			DEBUG('n', "FakeTCP::Receive: all data received successfully.\n");
			break;
		} 
		lastIndex = inMailHdr->sliceIndex;
	}

	DEBUG('n', "Data received has %d mails or %d bytes in total\n", inMailHdr->totalSlices, bytesRead);
	int length = 0;
	while (!mailList->IsEmpty() && !mailHdrList->IsEmpty()) {
		inMailHdr = (MailHeader *)mailHdrList->Remove();
		//int len += (*mailHdrList->Remove())->length;
		buffer =(char *)mailList->Remove();
		strncpy(into + length, buffer, inMailHdr->length);
		DEBUG('n', "Debug: buffer is %s, len is %d, buffer #%d hdr.length is %d\ninto is %s, into+length is %s\n", buffer, strlen(buffer), inMailHdr->sliceIndex, inMailHdr->length, into, into+length);
		length += inMailHdr->length;
		delete[] buffer;
	}
	DEBUG('n', "Debug: into is %s\n", into);

	delete mailList;
	delete mailHdrList;

	return bytesRead;

}

bool FakeTCP::SendACK(PacketHeader* inPktHdr, MailHeader* inMailHdr) {

	PacketHeader outPktHdr;
	MailHeader outMailHdr;

	outPktHdr.from = inPktHdr->to;
	outPktHdr.to = inPktHdr->from;
	outPktHdr.totalSlices = 1;
	outPktHdr.sliceIndex = inPktHdr->sliceIndex;
	outMailHdr.from = inMailHdr->from;
	outMailHdr.to = inMailHdr->from;
	outMailHdr.length = 5;
	outMailHdr.totalSlices = 1;
	outMailHdr.sliceIndex = inMailHdr->sliceIndex;
	char ack[5];
	sprintf(ack, "%5d", inMailHdr->sliceIndex);

	if (DebugIsEnabled('n')) {
		DEBUG('n', "Sending ACK=%s(atoi(ACK) = %d) ", ack, atoi(ack));
		DebugPrintHeader(outPktHdr, outMailHdr);
	}
	postOffice->Send(outPktHdr, outMailHdr, ack);
	fflush(stdout);
	return true;

}
