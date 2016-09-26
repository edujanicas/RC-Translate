// send.c

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

int main() {

    int fd, n, addrlen;
    struct sockaddr_in addr;
    struct hostent *hostptr;
    char buffer[128];

    fd = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket
    if (fd == -1) exit(1);

    hostptr = gethostbyname("localhost");
    if(hostptr == NULL) {
        perror("Could not find host");
        exit(1);
    }

    memset((void*)&buffer, (int)'\0', sizeof(buffer));
    memset((void*)&addr, (int)'\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ((struct in_addr *)(hostptr -> h_addr_list[0])) -> s_addr;
    addr.sin_port = htons(58000);

    n = sendto(fd, "This is an UDP echo server!\n", 28, 0, (struct sockaddr*)&addr, sizeof(addr));
    if (n == -1) exit(1); //error

    addrlen = sizeof(addr);

    n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*)&addr, &addrlen);
    if (n == -1) exit(1); //error

    write(1, "echo: ", 6);
    write(1, buffer, n);
    close(fd);

    hostptr = gethostbyaddr((char*)&addr.sin_addr, sizeof(struct in_addr), AF_INET);
    if (hostptr == NULL) printf("sent by [%s:%hu]\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    else printf("sent by [%s:%hu]", hostptr -> h_name, ntohs(addr.sin_port));

    exit(0);

}
