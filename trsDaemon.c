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
extern int errno;

#define max(A, B) ((A) > (B)?(A):(B))

int fd;

static void quitTRS(int signo) {
	printf("Will now kill TCS\n");
	close(fd);
	exit(0);
}

int main(int argc, char** argv) {

  int i, fd, n, addrlen, newfd, nw, ret;
  struct sockaddr_in addr;
  struct hostent *hostptr;
  struct hostent *h;
  struct in_addr *a;
  char buffer[128];
  char response[2048] = "";
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

	if(gethostname(buffer, 128)==-1) {
		perror("Could not get host name");
		exit(1);
	}
	printf("Official host name: %s\n", buffer);
	if((h=gethostbyname(buffer))==NULL) {
		perror("Could not get host IP");
		exit(1);
	}
	a=(struct in_addr*)h->h_addr_list[0];
	printf("Internet address: %s\n", inet_ntoa(*a));

  strcpy(language, argv[1]);

  if((old_handler = signal(SIGCHLD, SIG_IGN)) == SIG_ERR) exit(1);
	if((old_handler = signal(SIGINT, quitTRS))) exit(1);

  // INFORM TCS THAT IS NOW TRANSLATING
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

  strcpy(buffer, "SRG ");
  strcat(buffer, language);
	strcat(buffer, " ");
	strcat(buffer, inet_ntoa(*a));
	strcat(buffer, " ");
	strcat(buffer, TRSport);
  strcat(buffer, "\n");
  n = sendto(fd, buffer, strlen(buffer), 0, (struct sockaddr*)&addr, sizeof(addr));
  if (n == -1) exit(1); //error

  addrlen = sizeof(addr);

  n = recvfrom(fd, buffer, 128, 0, (struct sockaddr*)&addr, &addrlen);
  if (n == -1) exit(1); //error

  if((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) perror("Error creating socket");

  memset((void*)&addr, (int)'\0', sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(TRSport);

  if(bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    close(fd);
    perror("Could not bind");
  }

  if(listen(fd, 5) == -1) perror("Failed to listen");

  while(1) {

    memset((void*)&buffer, (int)'\0', sizeof(buffer));
    memset((void*)&response, (int)'\0', sizeof(response));
    addrlen = sizeof(addr);
    do newfd = accept(fd, (struct sockaddr*)&addr, &addrlen); //Wait for a connection
    while (newfd == -1 && errno == EINTR);
    printf("Accepted a connection\n");
    if(newfd == -1) perror("Could not accept connection");

    if((pid = fork()) == -1) perror("Error on fork");
    else if(pid == 0) { //Child process
      close(fd);
      while((n = read(newfd, buffer, 128)) != 0) {
        printf("Received message from: %s: %s", inet_ntoa(addr.sin_addr), buffer);
        if (n == -1) perror("Error while reading the message");
        trsCore(buffer, response, language);
        ptr = &response[0];
        n = strlen(response);
        printf("Sent message to: %s: %sSize: %d\n", inet_ntoa(addr.sin_addr), response, n);
        while (n > 0) {
          if ((nw = write(newfd, ptr, n)) <= 0) perror("Error retrieving message");
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
