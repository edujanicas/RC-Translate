#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include "tcsCore.c"

int main(int argc, char** argv) {

	int fd, addrlen, ret, nread, port = 58000;
	struct sockaddr_in addr;
	char buffer[512], response[2048];

	if(argc != 1 && argc != 3) {
		printf("Usage: ./TCS [-p TCSport]\n");
		exit(1);
	} else if (argc == 3) {
		port = *argv[2];
	}

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
		addrlen = sizeof(addr);
		nread = recvfrom(fd, buffer, 128, 0, (struct sockaddr*)&addr, &addrlen);
		if(nread == -1) perror("Error on receiving the message");
    printf("Received message from %s: %s", inet_ntoa(addr.sin_addr), buffer);

		// PROCESS THE INPUT MESSEGE AND FILL THE RESULT IN RESPONSE
    strcpy(response,tcsCore(buffer));

		// REPLY
		ret = sendto(fd, response, strlen(response), 0, (struct sockaddr*)&addr, addrlen);
		if(ret == -1) perror("Error echoing the answer");
        printf("Sent message to %s: %s", inet_ntoa(addr.sin_addr), response);


	}

	close(fd);
	exit(0);
}
