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
		int lastIndex;
		NetworkAddress destination;
		int totalLost;
		int totalReceive;
		float lostRate;

		// Private functions
		bool ReceiveACK(int index);
		bool SendACK(PacketHeader* inPktHdr, MailHeader* inMailHdr);
		int SendAll(char* data);
		void SendOne(PacketHeader outPktHdr, MailHeader outMailHdr, char* data);  // Send data of index bigger than *index*
};
#endif
