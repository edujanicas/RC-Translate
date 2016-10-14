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
#include <netdb.h>
#include "trsCore.c"
#define BUFFER_SIZE 512

#define max(A, B) ((A) > (B)?(A):(B))
#define min(A, B) ((A) < (B)?(A):(B))


// Global variables
extern int errno;
int fd, TRSport, TCSport, addrlen, n;
char TCSname[32], language[32];

// Function prototypes
int informTCS(int TRSport, int TCSport, char* TCSname, short option);
static void quitTRS(int signo);

int main(int argc, char** argv) {

	// Variable declarations
	int i, fd, n, addrlen, newfd, nwritten, nread, nleft, ret;
	struct sockaddr_in addr;
	char buffer[BUFFER_SIZE], response[BUFFER_SIZE] = "";
	char *ptr;
	int *doRead, *doWrite, *firstExec, *leftInFile;
	pid_t pid;
	char fileName[32];

	// Signal handling
	void (*old_handler)(int); //Interrupt handler
	if((old_handler = signal(SIGCHLD, SIG_IGN)) == SIG_ERR) exit(1);
	if((old_handler = signal(SIGINT, quitTRS))) exit(1);

	// Variable initialization
	TCSport = 58021;
	TRSport = 59021;
	strcpy(TCSname, "localhost");

	// Mallocs
	doRead = malloc(sizeof(int));
	doWrite = malloc(sizeof(int));
	firstExec = malloc(sizeof(int));
	leftInFile = malloc(sizeof(int));

	// Argument reading
	if(argc != 2 && argc != 4 && argc != 6 && argc != 8) {
		printf("Usage: ./TRS language [-p TRSport] [-n TCSname] [-e TCSport]\n");
		exit(1);
	}
	else if (argc == 4) {
		if (!strcmp(argv[2], "-p")) {
			TRSport = atoi(argv[3]);
		}
		else if (!strcmp(argv[2], "-n")) {
			strcpy(TCSname, argv[3]);
		}
		else if (!strcmp(argv[2], "-e")) {
			TCSport = atoi(argv[3]);
		}
		else {
			printf("Usage: ./TRS language [-p TRSport] [-n TCSname] [-e TCSport]\n");
			exit(1);
		}
	}
	else if (argc == 6) {
		for (i = 2; i < argc; i+=2){
			if (!strcmp(argv[i], "-p")) {
				TRSport = atoi(argv[2]);
			}
			else if (!strcmp(argv[i], "-n")) {
				strcpy(TCSname, argv[2]);
			}
			else if (!strcmp(argv[i], "-e")) {
				TCSport = atoi(argv[2]);
			}
			else {
				printf("Usage: ./TRS language [-p TRSport] [-n TCSname] [-e TCSport]\n");
				exit(1);
			}
		}
	}
	else if (argc == 8) {
		for (i = 2; i < argc; i+=2){
			if (!strcmp(argv[i], "-p")) {
				TRSport = atoi(argv[2]);
			}
			else if (!strcmp(argv[i], "-n")) {
				strcpy(TCSname, argv[2]);
			}
			else if (!strcmp(argv[i], "-e")) {
				TCSport = atoi(argv[2]);
			}
			else {
				printf("Usage: ./TRS language [-p TRSport] [-n TCSname] [-e TCSport]\n");
				exit(1);
			}
		}
	}
	strcpy(language, argv[1]);

	// TCP Connection
	if((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Error creating socket");
		exit(1);
	}

	memset((void*)&addr, (int)'\0', sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(TRSport);

	if(bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		perror("Could not bind");
		close(fd);
		exit(1);
	} // Error handling

	// Start by informing the TCS that is now providing translations
	n = informTCS(TRSport, TCSport, TCSname, 1);
	if (n == -1) {
		printf("TCS refused connection, will now exit.\n");
		exit(1);
	} // Error handling

	if(listen(fd, 5) == -1) {
		perror("Failed to listen");
		close(fd);
		exit(1);
	} // Error handling

	while(1) {
		// TCP SERVER

		memset((void*)&buffer, (int)'\0', BUFFER_SIZE);
		memset((void*)&response, (int)'\0', BUFFER_SIZE);
		addrlen = sizeof(addr);

		do newfd = accept(fd, (struct sockaddr*)&addr, (socklen_t *)&addrlen);
		while (newfd == -1 && errno == EINTR); //Wait for a connection

		if(newfd == -1) {
			perror("Could not accept connection");
		} else {
			printf("Connection from: %s\n", inet_ntoa(addr.sin_addr));
		}

		if((pid = fork()) == -1) perror("Error on fork");
		else if (pid == 0) { //Child process

			close(fd);

			memset(buffer, (int)'\0', BUFFER_SIZE); // initializing the buffer with /0
	        ptr = buffer;
	        nleft = BUFFER_SIZE;

	        *doRead = 0;
			*doWrite = 0;
			*firstExec = 0;

	        while((nread = read(newfd, buffer, BUFFER_SIZE)) != 0) {

				if (nread == -1) {
					perror("Error while reading the message");
					break;
				}

				printf("Received message from: %s: %s", inet_ntoa(addr.sin_addr), buffer);

				// Send to core to process message

				trsCore(buffer, response, language, fileName,
					doRead, doWrite, firstExec, leftInFile);
				if (*doRead) continue; // if the core puts this flag as 1, the daemon has to keep reading

				ptr = response;
				nleft = strlen(response);

				while (nleft > 0) {
					if ((nwritten = write(newfd, ptr, nleft)) <= 0) {
						perror("Error retrieving message");
						break;
					}
					nleft -= nwritten;
					ptr += nwritten;
					if ((*doWrite) && (nleft == 0)) {
						trsCore(buffer, response, language, fileName,
							doRead, doWrite, firstExec, leftInFile);
						ptr = response;
						nleft = min(BUFFER_SIZE, *leftInFile);
						if (nleft < 0) {
							nleft += BUFFER_SIZE;
						}
					}
				}
				printf("Sent message to: %s: %sSize: %lu\n", inet_ntoa(addr.sin_addr), response, strlen(response));

				close(newfd);
				exit(0);
			}
		}

		// Parent process
		do ret = close(newfd);
		while (ret == -1 && errno == EINTR);
		if(ret == -1) exit(1); //Error
	}

	close(fd);
	exit(0);
}

// Function declarations

int informTCS(int TRSport, int TCSport, char* TCSname, short option) {
	// 0 to leave, 1 to join
	int fdUDP;
	struct hostent *h;
	struct in_addr *a;
	struct sockaddr_in addr;
	char buffer[128], port[8];

	if(gethostname(buffer, 128)==-1) {
		perror("Could not get host name");
		exit(1);
	}
	//strcat(buffer, ".tecnico.ulisboa.pt");
	if((h=gethostbyname(buffer))==NULL) {
		perror("Could not get host IP");
		exit(1);
	}
	a=(struct in_addr*)h->h_addr_list[0];

	// INFORM TCS THAT IS NOT TRANSLATING
	fdUDP = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket
	if (fd == -1) exit(1);

	if (option == 1) {
		printf("Official host name: %s\n", buffer);
		printf("Internet address: %s\n", inet_ntoa(*a));
	}

	memset((void*)&buffer, (int)'\0', sizeof(buffer));
	memset((void*)&addr, (int)'\0', sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ((struct in_addr *)(h -> h_addr_list[0])) -> s_addr;
	addr.sin_port = htons(TCSport);

	if (option == 0) {
		strcpy(buffer, "SUN ");
	} else {
		strcpy(buffer, "SRG ");
	}
	strcat(buffer, language);
	strcat(buffer, " ");
	strcat(buffer, inet_ntoa(*a));
	strcat(buffer, " ");
	sprintf(port, "%d", TRSport);
	strcat(buffer, port);
	strcat(buffer, "\n");
	n = sendto(fdUDP, buffer, strlen(buffer), 0, (struct sockaddr*)&addr, sizeof(addr));
	if (n == -1) exit(1); //error

	addrlen = sizeof(addr);
	memset((void*)&buffer, (int)'\0', sizeof(buffer));

	n = recvfrom(fdUDP, buffer, 128, 0, (struct sockaddr*)&addr, (socklen_t *)&addrlen);
	if (n == -1) exit(1); //error

	close(fdUDP);

	if(!strcmp(buffer, "SRR NOK\n")) {
		close(fd);
		printf("Wasn't accepted as a valid server\n");
		exit(0);
	}
	if(!strcmp(buffer, "SUR NOK\n")) {
		close(fd);
		printf("Tried to disconnect from a wrong TCS\n");
		exit(0);
	}

	return 1;

}

static void quitTRS(int signo) {
	informTCS(TRSport, TCSport, TCSname, 0);
	close(fd);
	printf("Bye!\n");
	exit(0);
}
