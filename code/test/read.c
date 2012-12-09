/*
 * read.c
 *
 *  Created on: 2012-11-18
 *      Author: rye
 */
int main() {
	//Test Create
		int fd;
		char buffer[100];
		int readSize;

		fd = Open("TestSysCall");
		readSize = Read(buffer, 21, fd);
		Close(fd);
		Print("Bytes read is %d",readSize);
		Halt();
}



