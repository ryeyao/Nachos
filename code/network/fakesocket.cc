// fakesocket.cc
//  Implement a fake socket for nachos
//
//  Created on: 2012-12-14
//      Author: rye

#include "fakesocket.h"

FakeSocket::FakeSocket(NetworkAddress dest, FakeSocketType type) {

	this->type = type;
	fakeTCP = new FakeTCP(dest);

}

FakeSocket::~FakeSocket() {
	delete fakeTCP;

}

int FakeSocket::Send(char* data) {

	ASSERT(data != NULL);
	DEBUG('n', "Sending data using FakeSocket::Send\n");
	if (type == FAKE_TCP) {
		return fakeTCP->Send(data);
	}
	else {
	    DEBUG('n', "Invalid Socket Type!\n");
		return 0;
	}
}

int FakeSocket::Receive(char* into, int numBytes) {

	if (type == FAKE_TCP) {
		return fakeTCP->Receive(into);
	}
	else {
	    DEBUG('n', "Invalid Socket Type!\n");
		return 0;
	}
}
