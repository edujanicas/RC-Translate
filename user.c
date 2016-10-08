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
#include <signal.h>

#define False 0
#define True 1

int connectTRS(char* message);
int countWords(char* s);

int main(int argc, char** argv) {

    int fdUDP, n, addrlen, numLang, i = 1;
    struct sockaddr_in addr;
    struct hostent *hostptr;
    char buffer[128];
    char TCSname[32] = "localhost";
    char instruction[32];
    char languages[32][99];
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
    fdUDP = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket
    if (fdUDP == -1) exit(1);

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

            n = sendto(fdUDP, "ULQ\n", 4, 0, (struct sockaddr*)&addr, sizeof(addr));
            if (n == -1) exit(1); //error

            addrlen = sizeof(addr);
            n = recvfrom(fdUDP, buffer, 128, 0, (struct sockaddr*)&addr, &addrlen);
            if (n == -1) exit(1); //error

            if (strcmp(strtok(buffer, " "), "ULR")){
                printf("TCS server error. Repeat request.");
                break;
            }
            numLang = atoi(strtok(NULL, " "));
            while (i <= numLang){
                strcpy(languages[i-1], strtok(NULL, " "));
                printf("%d- %s\n", i, languages[i-1]);
                i++;
            }
            fflush(stdout);

        } else if (!strcmp(instruction, "request")){

            strcpy(instruction, "UNQ \0");
            scanf("%d", &i);

            strcat(instruction, languages[i-1]);
            strcat(instruction, "\n");

            n = sendto(fdUDP, instruction, strlen(instruction), 0, (struct sockaddr*)&addr, sizeof(addr));
            if (n == -1) exit(1); //error

            addrlen = sizeof(addr);
            n = recvfrom(fdUDP, buffer, 128, 0, (struct sockaddr*)&addr, &addrlen);
            if (n == -1) exit(1); //error
            connectTRS(buffer);


        } else {
            printf("Available commands are: list, request\n");
        }

        printf(">> ");
        fflush(stdout);

        scanf("%s", instruction);
    }
    close(fdUDP);
    /*
    hostptr = gethostbyaddr((char*)&addr.sin_addr, sizeof(struct in_addr), AF_INET);
    if (hostptr == NULL) printf("sent by [%s:%hu]\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    else printf("sent by [%s:%hu]", hostptr -> h_name, ntohs(addr.sin_port));
    */
    exit(0);
}

int connectTRS(char* message){
    // message is the info necessary to connect to TRS, given by TCS
    void (*old_handler)(int); //interrupt hanlder

    if ((old_handler = signal(SIGPIPE, SIG_IGN)) == SIG_ERR) exit(1); // Error

    int fdTCP, n, nbytes, nleft, nwritten, nread, nWords, i=0;
    struct sockaddr_in addr;
    char *ptr;
    char TRSname[32] = "localhost";
    char request[8] = "TRQ ";
    char buffer[320];
    char words_str[320];
    char type[8];
    char* token;
    int TRSport = 59000;

    fdTCP = socket(AF_INET, SOCK_STREAM, 0); // TCP Socket
    if (fdTCP == -1) perror("Failed initializing socket");

    if (strcmp(strtok(message, " "), "UNR")){
        printf("TCS server error. Repeat request.");
        exit(1);
    }

    strcpy(TRSname, strtok(NULL, " ")); // getting the IP address of TRS from message
    TRSport = atoi(strtok(NULL, " ")); // the TRS port from message

    memset((void*)&addr, (int)'\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(TRSname);
    addr.sin_port = htons(TRSport);

    n = connect(fdTCP, (struct sockaddr*)&addr, sizeof(addr)); // connect to TRS
    if (n == -1) perror("Failed to connect");

    printf("%s %d\n", TRSname, TRSport); // print TRS info

    memset(buffer, (int)'\0', 320); // initializing the buffer with /0
    strcpy(buffer, request);

    fgets(words_str, 319, stdin); // getting the info given by the user to send to TRS
    // fgets keeps the spaces !!!

    n = sscanf(words_str, "%s", type);

    // the type has to be text(t) or file (f)
    if (!strcmp(type, "f")){


    }

    else if (!strcmp(type, "t")){
        nWords = countWords(words_str) - 1; // remove type from count

        strcat(buffer, type);
        strcat(buffer, " ");

        sprintf(type, "%d", nWords);
        strcat(buffer, type);

        strcat(buffer, &words_str[2]);
    }

    else{
        printf("Usage: request n t W1 W2 ... Wn OR request n f filename\n");
        return 1;
    }


    ptr = buffer;
    nbytes = strlen(buffer);

    nleft = nbytes;

    while(nbytes > 0) {
        nwritten = write(fdTCP, ptr, nleft);
        if (nwritten <= 0) perror("Falha a enviar mensagem");
        nbytes -= nwritten;
        ptr += nwritten;
    }

    memset(buffer, (int)'\0', 320); // initializing the buffer with /0


    nleft = 320;
    ptr = buffer;
    while(nleft > 0) {
        nread = read(fdTCP, ptr, nleft);
        if (nread == -1) perror("Falha a ler mensagem");
        else if (nread == 0) break; // Closed by peer
        nleft -= nread;
        ptr += nread;
    }

    strcpy(words_str, buffer);

    memset(buffer, (int)'\0', 320); // initializing the buffer with /0

    token = strtok(words_str, " "); // use this to check for error
    token = strtok(NULL, " "); // ditch the type
    token = strtok(NULL, " "); // ditch the numWords

    while (i < nWords){
                strcat(buffer, strtok(NULL, " "));
                strcat(buffer, " ");
                i++;
            }

    printf("%s: %s \n", TRSname, buffer);

    close(fdTCP);
    return 0;
}

int countWords(char* s){
 int count = 0, i;
 int foundLetter = False;
 for (i = 0;i<strlen(s);i++)
 {
  if (s[i] == ' ')
      foundLetter = False;
  else
  {
      if (foundLetter == False)
          count++;
      foundLetter = True;
  }
 }
 return count;
}
