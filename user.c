#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

int main(int argc, char** argv) {

    int fd, n, addrlen;
    struct sockaddr_in addr;
    struct hostent *hostptr;
    char buffer[128];
    char TCSname[32] = "localhost";
    char instruction[32];
    char language[32];
    int TCSport = 58000;

    // Argument reading
    if(argc != 1 && argc != 3 && argc != 5) {
		printf("Usage: ./user [-n TCSname] [-p TCSport]\n");
		exit(1);
	} else if (argc == 3) {
		if (!strcmp(argv[1], "-n")) {
            strcpy(TCSname, argv[2]);
        } else if (!strcmp(argv[1], "-p")) {
            TCSport = atoi(argv[2]);
        } else {
            printf("Usage: ./user [-n TCSname] [-p TCSport]\n");
    		exit(1);
        }
	} else if (argc == 5) {
        if (!strcmp(argv[1], "-n") && !strcmp(argv[3], "-p")) {
            strcpy(TCSname, argv[2]);
            TCSport = atoi(argv[4]);
        } else if (!strcmp(argv[1], "-p") && !strcmp(argv[3], "-n")) {
            strcpy(TCSname, argv[4]);
            TCSport = atoi(argv[2]);
        } else {
            printf("Usage: ./user [-n TCSname] [-p TCSport]\n");
    		exit(1);
        }
    }

    // UDP connection
    fd = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket
    if (fd == -1) exit(1);

    hostptr = gethostbyname(TCSname);
    if(hostptr == NULL) {
        perror("Could not find host");
        exit(1);
    }

    memset((void*)&buffer, (int)'\0', sizeof(buffer));
    memset((void*)&addr, (int)'\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ((struct in_addr *)(hostptr -> h_addr_list[0])) -> s_addr;
    addr.sin_port = htons(TCSport);

    printf(">> ");
    fflush(stdout);

    scanf("%s", instruction);
    while(strcmp(instruction, "exit")) {

        if(!strcmp(instruction, "list")) {

            n = sendto(fd, "ULQ\n", 4, 0, (struct sockaddr*)&addr, sizeof(addr));
            if (n == -1) exit(1); //error

            addrlen = sizeof(addr);
            n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*)&addr, &addrlen);
            if (n == -1) exit(1); //error

            write(1, buffer, n);
            fflush(stdout);

        } else if (!strcmp(instruction, "request")){

            strcpy(instruction, "UNQ \0");

            scanf("%s", language);

            strcat(instruction, language);
            strcat(instruction, "\n");

            n = sendto(fd, instruction, strlen(instruction), 0, (struct sockaddr*)&addr, sizeof(addr));
            if (n == -1) exit(1); //error

            addrlen = sizeof(addr);
            n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*)&addr, &addrlen);
            if (n == -1) exit(1); //error

            write(1, buffer, n);
            fflush(stdout);

        } else {
            printf("Available commands are: list, request\n");
        }

        printf(">> ");
        fflush(stdout);

        scanf("%s", instruction);
    }
    close(fd);

    /*
    hostptr = gethostbyaddr((char*)&addr.sin_addr, sizeof(struct in_addr), AF_INET);
    if (hostptr == NULL) printf("sent by [%s:%hu]\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    else printf("sent by [%s:%hu]", hostptr -> h_name, ntohs(addr.sin_port));
    */

    exit(0);

}
