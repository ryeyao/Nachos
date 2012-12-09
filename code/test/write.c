/*
 * write.c
 *
 *  Created on: 2012-11-18
 *      Author: rye
 */
int main() {
	//Test Create
		int fd;
		int readSize;


		Create("TestSysCall");
		fd = Open("TestSysCall");
		Write("Test my system call.",21,fd);
		Close(fd);
		Halt();
}
