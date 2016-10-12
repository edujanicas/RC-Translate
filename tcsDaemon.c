#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "tcsCore.c"

int fd;

static void quitTCS(int signo) {
	printf("Will now kill TCS\n");
	close(fd);
	exit(0);
}

int main(int argc, char** argv) {

	int addrlen, ret, nread, port = 58021, *nServers;
	struct sockaddr_in addr;
	struct hostent *h;
	struct in_addr *a;
	char buffer[512];
	char domain[32];
	char response[2048];
	void (*old_handler)(int); //Interrupt handler

	if((old_handler = signal(SIGINT, quitTCS))) exit(1);

	if(argc != 1 && argc != 3) {
		printf("Usage: ./TCS [-p TCSport]\n");
		exit(1);
	} else if (argc == 3) {
		port = *argv[2];
	}

	nServers = malloc(sizeof(int));
	*nServers = 0;

	if(gethostname(buffer, 512)==-1) {
		perror("Could not get host name");
		exit(1);
	}
	if(getdomainname(domain, 32)==-1) {
		perror("Could not get domain name");
		exit(1);
	}
	strcat(buffer, domain);
	printf("Official host name: %s\n", buffer);
	if((h=gethostbyname(buffer))==NULL) {
		perror("Could not get host IP");
		exit(1);
	}
	a=(struct in_addr*)h->h_addr_list[0];
	printf("Internet address: %s\n", inet_ntoa(*a));

	if((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1 ) perror("Error creating socket");

  memset((void*)&response, (int)'\0', sizeof(response));
	memset((void*)&addr, (int)'\0', sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	ret = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
	if (ret == -1) perror("Could not bind");

	while(1) {

		// RECEIVE
		memset((void*)&buffer, (int)'\0', sizeof(buffer));
		memset((void*)&response, (int)'\0', sizeof(response));

		addrlen = sizeof(addr);
		nread = recvfrom(fd, buffer, 128, 0, (struct sockaddr*)&addr, (socklen_t *)&addrlen);
		if(nread == -1) perror("Error on receiving the message");
    printf("Received message from %s: %s", inet_ntoa(addr.sin_addr), buffer);

		// PROCESS THE INPUT MESSEGE AND FILL THE RESULT IN RESPONSE
    		tcsCore(buffer, response, nServers);
		// REPLY
		ret = sendto(fd, response, strlen(response), 0, (struct sockaddr*)&addr, addrlen);
		if(ret == -1) perror("Error echoing the answer");
    printf("Sent message to %s: %s", inet_ntoa(addr.sin_addr), response);


	}
	exit(0);
}
