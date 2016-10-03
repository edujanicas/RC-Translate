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

int main(int argc, char** argv) {

    int i, fd, n, addrlen, newfd, nw, ret;
    struct sockaddr_in addr;
    struct hostent *hostptr;
    char buffer[128];
    char TCSname[32] = "localhost";
    char instruction[32];
    char language[32];
    int TCSport = 58000;
    int TRSport = 59000;
	char *ptr;
	pid_t pid;
	void (*old_handler)(int); //Interrupt handler

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
