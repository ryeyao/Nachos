// fakesocket.h
//  Implement a fake socket for nachos
//
//  Created on: 2012-12-14
//      Author: rye

#ifndef FAKESOCKET_H_
#define FAKESOCKET_H_
#endif

#include "post.h"

#define ACK '1'
enum FakeSocketType {
	FAKE_TCP,
	FAKE_UDP
};

class FakeSocket {

	public:
		NetworkAddress to;
		NetworkAddress from;
		
		FakeSocket(NetworkAddress to, FakeSocketType type); // Constructor
		int Send(char* data); // Send data, return number of bytes that have been sent.
		int Receive(char* into, int numBytes); // Receive *numBytes* of data into buffer *into*, return number of bytes that have been actually received

	private:
		List* mailList;
		List* mailHdrList;
		PacketHeader outPktHdr;
		PacketHeader inPktHdr;
		MailHeader outMailHdr;
		MailHeader inMailHdr;
		void ReceiveACK();
		void SendACK();

};
