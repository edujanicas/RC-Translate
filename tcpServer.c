#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
extern int errno;

#define max(A, B) ((A) > (B)?(A):(B))

int main() {

	int fd, addrlen, newfd, n, nw, ret;
	struct sockaddr_in addr;
	char *ptr, buffer[128];
	pid_t pid;
	void (*old_handler)(int); //Interrupt handler

	if((old_handler = signal(SIGCHLD, SIG_IGN)) == SIG_ERR) exit(1);
	
	if((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) perror("Error creating socket");

	memset((void*)&addr, (int)'\0', sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(58000);

	if(bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		close(fd);
		perror("Could not bind");
	}

	if(listen(fd, 5) == -1) perror("Failed to listen");
	
	while(1) {
		addrlen = sizeof(addr);
		do newfd = accept(fd, (struct sockaddr*)&addr, &addrlen); //Wait for a connection
		while (newfd == -1 && errno == EINTR);
		if(newfd == -1) perror("Could not accept connection");
		
		if((pid = fork()) == -1) perror("Error on fork");
		else if(pid == 0) { //Child process	
			close(fd);
			while((n = read(newfd, buffer, 128)) != 0) {
				if (n == -1) perror("Error while reading the message");
				ptr = &buffer[0];
				while (n > 0) {
					if ((nw = write(newfd, ptr, n)) <= 0) perror("Error echoing message");
					n -= nw;
					ptr += nw;
				}
			}
			close(newfd);
			exit(0);
		}
		// Parent process
		do ret = close(newfd);
		while (ret == -1 && errno == EINTR);
		if(ret == -1) exit(1); //Error
	}
	
	close(fd);
	exit(0);
}
