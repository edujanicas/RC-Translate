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

#define FALSE 0
#define TRUE 1
#define FileBufferSIZE 1000000
#define SEPARATOR " \n"

// Function prototypes
int connectTRS(char* message);
int countWords(char* s);

int main(int argc, char** argv) {

    // Variable declarations
    int fdUDP, n, addrlen, numLang, i, TCSport = 58000;
    struct sockaddr_in addr;
    struct hostent *hostptr;
    char buffer[128], instruction[32], TCSname[32] = "localhost";
    char languages[32][99];

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
    if (fdUDP == -1) {
		perror("Error creating socket");
		exit(1);
	} // Error handling

    hostptr = gethostbyname(TCSname);
    if(hostptr == NULL) {
        perror("Could not find host");
		close(fdUDP);
        exit(1);
    } // Error handling

    memset((void*)&buffer, (int)'\0', sizeof(buffer));
    memset((void*)&addr, (int)'\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ((struct in_addr *)(hostptr -> h_addr_list[0])) -> s_addr;
    addr.sin_port = htons(TCSport);

    // Process input
    printf(">> ");
    fflush(stdout);

    scanf("%s", instruction);

    while(strcmp(instruction, "exit")) {

        if(!strcmp(instruction, "list")) {
            // The user will send an ULQ message
            // The response will be of format ULR nL L1 L2 ... Ln

            n = sendto(fdUDP, "ULQ\n", 4, 0, (struct sockaddr*)&addr, sizeof(addr));
            if (n == -1) {
                perror("Failed to send message to TCS");
                exit(1);
            } // Error handling

            addrlen = sizeof(addr);
            n = recvfrom(fdUDP, buffer, 128, 0, (struct sockaddr*)&addr, (socklen_t*)&addrlen);
            if (n == -1) {
                perror("Failed to receive message from TCS");
                exit(1);
            } // Error handling

            if (strcmp(strtok(buffer, SEPARATOR), "ULR")){
                printf("TCS server error. Repeat request.");
                break;
            } // Error handling

            numLang = atoi(strtok(NULL, SEPARATOR));
            i = 1;

            printf("\n----- Available languages: -----\n\n");
            while (i <= numLang){
                strcpy(languages[i-1], strtok(NULL, SEPARATOR));
                printf("%d -> %s\n", i, languages[i-1]);
                i++;
            }
            printf("\n--------------------------------\n\n");

            fflush(stdout);

        } else if (!strcmp(instruction, "request")){
            // The user will send an URQ Ln message
            // The response will be of format UNR IPTRS portTRS

            strcpy(instruction, "UNQ \0");
            scanf("%d", &i);

            strcat(instruction, languages[i-1]);
            strcat(instruction, "\n");

            n = sendto(fdUDP, instruction, strlen(instruction), 0, (struct sockaddr*)&addr, sizeof(addr));
            if (n == -1) exit(1); //error

            addrlen = sizeof(addr);
            n = recvfrom(fdUDP, buffer, 128, 0, (struct sockaddr*)&addr, (socklen_t*)&addrlen);
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
    exit(0);
}

int connectTRS(char* message){
    // message is the info necessary to connect to TRS, given by TCS
    void (*old_handler)(int); //interrupt hanlder

    if ((old_handler = signal(SIGPIPE, SIG_IGN)) == SIG_ERR) exit(1); // Error

    int fdTCP, n, nbytes, nleft, nwritten, nread, nWords, size, sizeBuffer, i=0;
    struct sockaddr_in addr;
    char *ptr;
    char TRSname[32] = "localhost";
    char request[8] = "TRQ ";
    char* buffer;
    char* fileStr;
    char userInput[320];
    char type[8];
    char nWordsStr[8];
    char fileLengthStr[8];
    char* token;
    int TRSport = 59000;
    FILE *file;
    char filename[32];
    int fileLength;

    fdTCP = socket(AF_INET, SOCK_STREAM, 0); // TCP Socket
    if (fdTCP == -1) perror("Failed initializing socket");

    if (strcmp(strtok(message, SEPARATOR), "UNR")){
        printf("TCS server error. Repeat request.");
        exit(1);
    }

    strcpy(TRSname, strtok(NULL, SEPARATOR)); // getting the IP address of TRS from message
    TRSport = atoi(strtok(NULL, SEPARATOR)); // the TRS port from message

    memset((void*)&addr, (int)'\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(TRSname);
    addr.sin_port = htons(TRSport);

    n = connect(fdTCP, (struct sockaddr*)&addr, sizeof(addr)); // connect to TRS
    if (n == -1) { perror("Failed to connect"); exit(1);}

    printf("%s %d\n", TRSname, TRSport); // print TRS info

    fgets(userInput, 319, stdin); // getting the info given by the user to send to TRS
    // fgets keeps the spaces !!!

    n = sscanf(userInput, "%s", type);

    // the type has to be text(t) or file (f)
    if (!strcmp(type, "f")){
        n = sscanf(userInput, "%s" "%s", type, filename); // get the filename

        //open the image file in binary
        if ((file = fopen(filename, "rb")) == NULL){
            printf("Error opening file\n");
            exit(1);} // error

        fseek(file, 0, SEEK_END); // Seek to the end of the file
        fileLength = ftell(file); // Get the size from the end position
        rewind(file);             // Go back to the start of the file

        size = FileBufferSIZE;
        buffer = (char*) malloc (sizeof(char)*size);
        fileStr = (char*) malloc (sizeof(char)*fileLength);
        memset(buffer, (int)'\0', size); // initializing the buffer with /0

        strcpy(buffer, request); // put the request code in the buffer
        strcat(buffer, type);
        strcat(buffer, " ");
        strcat(buffer, filename);
        strcat(buffer, " ");
        sprintf(fileLengthStr, "%d", fileLength);
        strcat(buffer, fileLengthStr);
        strcat(buffer, " ");

        if ((nWords= fread(fileStr, 1, fileLength, file)) != fileLength) { // Reads, stores in buffer and returns the total number of elements successfully read
            printf("Error reading file");
            exit(1); //error reading file
        }
        sizeBuffer = strlen(buffer) + fileLength + 1;
        strcat(buffer, fileStr);
        strcat(buffer, "\n");


        }

    else if (!strcmp(type, "t")){
        nWords = countWords(userInput) - 1; // remove type from count
        size = 320;
        buffer = (char*) malloc (sizeof(char)*size);
        memset(buffer, (int)'\0', size); // initializing the buffer with /0

        strcpy(buffer, request); // put the request code in the buffer
        strcat(buffer, type);
        strcat(buffer, " ");



        sprintf(nWordsStr, "%d", nWords);
        strcat(buffer, nWordsStr);

        strcat(buffer, &userInput[2]);
        sizeBuffer = strlen(buffer);
    }

    else{
        printf("Usage: request n t W1 W2 ... Wn OR request n f filename\n");
        return 1;
    }

    ptr = buffer;
    nbytes = sizeBuffer;

    nleft = nbytes;

    while(nleft > 0) {
        nwritten = write(fdTCP, ptr, nleft);
        if (nwritten <= 0) perror("Falha a enviar mensagem");
        nleft -= nwritten;
        ptr += nwritten;
    }

    printf("Sent.\n");
    nleft = size;

    memset(buffer, (int)'\0', size); // initializing the buffer with /0
    ptr = buffer;

    while(nleft > 0) {
        nread = read(fdTCP, ptr, nleft);
        printf("%d\n", nread);
        if (nread == -1) perror("Falha a ler mensagem");
        else if (nread == 0) break; // Closed by peer
        nleft -= nread;
        ptr += nread;
    }
    if (nleft <= 0 || !(buffer)){
        printf("Not able to receive all data. Repeat request.\n");
        return 1;
    }

    printf("Received.\n");

    if (!strcmp(type, "f")){

        token = strtok(buffer, SEPARATOR); // request: use this to check for error
        token = strtok(NULL, SEPARATOR); // ditch the type
        strcpy(filename, strtok(NULL, SEPARATOR)); // get the filename
        size = atoi(strtok(NULL, SEPARATOR)); // get the file size

        file = fopen(filename, "wb");
        fwrite(strtok(NULL, SEPARATOR), 1, size, file);
        fclose(file);

    }

    else if (!strcmp(type, "t")){
        strcpy(userInput, buffer);

        memset(buffer, (int)'\0', size); // initializing the buffer with /0

        token = strtok(userInput, SEPARATOR); // TRR

        token = strtok(NULL, SEPARATOR); // error message if present, if not, ditch the type
        printf("%s\n", token );
        if (!strcmp(token, "ERR")){              //Check for error
            printf("Error\n");
            return 1;
        }
         else if (!strcmp(token, "NTA")){              //Check for error
            printf("No translation.\n");
            return 1;
        }
        token = strtok(NULL, SEPARATOR); // ditch the numWords

        while (i < nWords){
                    strcat(buffer, strtok(NULL, SEPARATOR));
                    strcat(buffer, " ");
                    i++;
                }
        printf("%s: %s \n", TRSname, buffer);
        }




    close(fdTCP);
    return 0;
}

int countWords(char* s){
 int count = 0, i;
 int foundLetter = FALSE;
 for (i = 0;i<strlen(s);i++)
 {
  if (s[i] == ' ')
      foundLetter = FALSE;
  else
  {
      if (foundLetter == FALSE)
          count++;
      foundLetter = TRUE;
  }
 }
 return count;
}
