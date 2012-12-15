// fakesocket.h
//  Implement a fake socket for nachos
//
//  Created on: 2012-12-14
//      Author: rye

#ifndef FAKESOCKET_H_
#define FAKESOCKET_H_
#include "faketcp.h"

enum FakeSocketType {
	FAKE_TCP,
	FAKE_UDP
};

class FakeSocket {

	public:
		FakeSocket(NetworkAddress dest, FakeSocketType type); // Constructor
		~FakeSocket();
		int Send(char* data); // Send data, return number of bytes that have been sent.
		int Receive(char* into, int numBytes); // Receive *numBytes* of data into buffer *into*, return number of bytes that have been actually received

	private:
		FakeSocketType type;
		FakeTCP *fakeTCP;

};
#endif
