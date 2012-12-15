// faketcp.h
//  Implement a fake TCP protocol for nachos
//
//  Created on: 2012-12-14
//      Author: rye

#ifndef FAKETCP_H_
#define FAKETCP_H_

#define ACK_1 '1'
#define MAX_TRY 16

#include "post.h"

class FakeTCPHeader {
	public:
		NetworkAddress to;
		NetworkAddress from;
};

class FakeTCP {

	public:
		FakeTCP(NetworkAddress dest);
		~FakeTCP();
		int Send(char* data);
		int Receive(char* into);

		
	private:
		List* mailList;   // Hold the mail that has been received
		List* mailHdrList;  // Mapped mail with mail header
		char** dataSent;   // Buffered data that was just sent for future use
		int* dataLengths;  // Buffered data length that was just sent for future use
		PacketHeader outPktHdr;
		PacketHeader inPktHdr;
		MailHeader outMailHdr;
		MailHeader inMailHdr;
		int lastIndex;

		// Private functions
		bool ReceiveACK(int index);
		bool SendACK(int index);
		int SendAll(char* data);
		void SendOne(int index);  // Send data of index bigger than *index*
};
#endif
