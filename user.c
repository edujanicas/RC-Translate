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
#define BUFFER_SIZE 512
#define SEPARATOR " \n"

// Function prototypes
int connectTRS(char* message);
int countWords(char* s);
int sendFile(int fd, char* userInput);

int main(int argc, char** argv) {

    // Variable declarations
    int fdUDP, n, addrlen, numLang, i, TCSport = 58021;
    struct sockaddr_in addr;
    struct hostent *hostptr;
    char buffer[128], instruction[32], TCSname[32] = "localhost";
    char languages[32][99];
	char *token;

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

			token = strtok(NULL, SEPARATOR); // token will retain the number of languages
			if (!token) {
				printf("Error on TCS reply, please try again\n");
				continue;
			} // Error handling
			numLang = atoi(token);
            i = 1;

            printf("\n----- Available languages: -----\n\n");
            while (i <= numLang){
				token = strtok(NULL, SEPARATOR);
				if (!token) {
					printf("Error on TCS reply, please try again\n");
					continue;
				} // Error handling
                strcpy(languages[i-1], token);
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
            printf("Available commands are: list, request, exit\n");
        }

        printf(">> ");
        fflush(stdout);

        scanf("%s", instruction);
    }

    close(fdUDP);
    exit(0);
}


/*****************************************************************************
                            FUNCTION connectTRS
Receives the request answer with the TRS info from the TCS and creates a TCP
socket to the TRS. Processes the request if text translation, if file translation
requested it calls function sendFile.
Returns 0 if successful and 1 otherwise.
******************************************************************************/

int connectTRS(char* message){
    // message is the info necessary to connect to TRS, given by TCS
    void (*old_handler)(int); //interrupt handler

    if ((old_handler = signal(SIGPIPE, SIG_IGN)) == SIG_ERR) exit(1); // Error

    int fdTCP, n, nleft, nwritten, nread, nWords, i=0;
    struct sockaddr_in addr;
    char *ptr;
    char TRSname[32] = "localhost";
    char request[8] = "TRQ ";
    char buffer[512];
    char userInput[320];
    char type[8];
    char nWordsStr[8];
    char* token;
    int TRSport = 59021;

    // TCP Socket
    if ((fdTCP = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("Failed initializing socket");
        return 1;
    }

    if (strcmp(strtok(message, SEPARATOR), "UNR")){
        perror("Bad request");
        return 1;
    }

    strcpy(TRSname, strtok(NULL, SEPARATOR)); // getting the IP address of TRS from message
    TRSport = atoi(strtok(NULL, SEPARATOR)); // the TRS port from message

    memset((void*)&addr, (int)'\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(TRSname);
    addr.sin_port = htons(TRSport);

    // connect to TRS
    if ((n = connect(fdTCP, (struct sockaddr*)&addr, sizeof(addr))) == -1) {
        perror("Failed to connect");
        return 1;
    }

    printf("%s %d\n", TRSname, TRSport); // print TRS info

    fgets(userInput, 319, stdin); // getting the info given by the user to send to TRS
    // fgets keeps the spaces !!!

    n = sscanf(userInput, "%s", type);

    // the type has to be text(t) or file (f)
    // if file, it's dealt by function sendFile
    if (!strcmp(type, "f")){
        if (sendFile(fdTCP, userInput)!=0) {
            perror("Error at function sendFile");
            close(fdTCP);
            return 1;
		}
    }

    else if (!strcmp(type, "t")){
        nWords = countWords(userInput) - 1; // remove type from count
        memset(buffer, (int)'\0', BUFFER_SIZE); // initializing the buffer with /0

        strcpy(buffer, request); // put the request code in the buffer
        strcat(buffer, type);
        strcat(buffer, " ");

        sprintf(nWordsStr, "%d", nWords);
        strcat(buffer, nWordsStr);

        strcat(buffer, &userInput[2]);

        // Send translation request and text to translate to TRS
        ptr = buffer;
        nleft = strlen(buffer);

        while(nleft > 0) {
            nwritten = write(fdTCP, ptr, nleft);
            if (nwritten == -1){
                perror("Failed to write text to TRS");
                return 1;
            }
            nleft -= nwritten;
            ptr += nwritten;
        }

        printf("Text to translate sent.\n");

        // Receive answer from TRS
        memset(buffer, (int)'\0', BUFFER_SIZE); // initializing the buffer with /0
        ptr = buffer;
        nleft = BUFFER_SIZE;

        while(nleft > 0) {
            nread = read(fdTCP, ptr, nleft);
            if (nread == -1){
                perror("Failed to read text translation from TRS");
                return 1;
            }
            else if (nread == 0) break; // Closed by peer
            nleft -= nread;
            ptr += nread;
        }

        if (nleft <= 0){
            perror("Failed to receive all data");
            return 1;
        }

        printf("Text translation received.\n");

        strcpy(userInput, buffer);

        memset(buffer, (int)'\0', BUFFER_SIZE); // initializing the buffer with /0

        token = strtok(userInput, SEPARATOR); // TRR

        token = strtok(NULL, SEPARATOR); // error message if present, if not, ditch the type

        if (!strcmp(token, "ERR")){
            perror("TRS: Bad request");
            return 1;
        }
         else if (!strcmp(token, "NTA")){
            perror("TRS: No translation");
            return 1;
        }

        token = strtok(NULL, SEPARATOR); // ditch the numWords

        while (i < nWords){
                    strcat(buffer, strtok(NULL, SEPARATOR));
                    strcat(buffer, " ");
                    i++;
                }
        printf("%s: %s \n", TRSname, buffer);

        close(fdTCP);
        return 0;
    }
    else{
        printf("Usage: request n t W1 W2 ... Wn OR request n f filename\n");
        close(fdTCP);
        return 1;
    }
	return 1;
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



/**********************************************************************
                        FUNCTION sendFile
receives the open file descriptor (fd) to send a file to, and the data
provided by the user (userInput) which consists of the type, file name,
file size and file data.
Returns 0 if successful and 1 otherwise.
***********************************************************************/

int sendFile(int fd, char* userInput){
    int n = 2, nleft, nwritten, nread, sizeData, indexData;
    char request[8] = "TRQ ";
    char* buffer;
    char type[8];
    char fileLengthStr[8];
    FILE *file;
    char fileName[32];
    char fileData[BUFFER_SIZE];
    int fileLength;
    char* token;
    char* ptr;

    if (sscanf(userInput, "%s" "%s", type, fileName) != n){   // get the filename
        printf("Usage: request n t W1 W2 ... Wn OR request n f filename\n");
        perror("Failed to get filename");
        return 1;
    }

    if ((file = fopen(fileName, "rb")) == NULL){ //open the image file in binary
        perror("Failed to open file");
        return 1;
    }

    fseek(file, 0, SEEK_END); // Seek to the end of the file
    fileLength = ftell(file); // Get the size from the end position
    rewind(file);             // Go back to the start of the file

    buffer = (char*) malloc (sizeof(char)*BUFFER_SIZE);
    memset(buffer, (int)'\0', BUFFER_SIZE); // initializing the buffer with /0

    strcpy(buffer, request); // put the request code in the buffer
    strcat(buffer, type);
    strcat(buffer, " ");
    strcat(buffer, fileName);
    strcat(buffer, " ");
    sprintf(fileLengthStr, "%d", fileLength);
    strcat(buffer, fileLengthStr);
    strcat(buffer, " ");

    printf("%d Bytes to transmit\n", fileLength);

    /************ SENDING THE FILE TO TRS **************/
    // First, send the request code, type, file name and file length first

    ptr = buffer;
    nleft = strlen(buffer);

    while(nleft > 0) {
        nwritten = write(fd, ptr, nleft);
        if (nwritten <= 0){
            perror("Failed to send request to TRS");
            return 1;
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    // Next, read the file in chunks of BUFFER_SIZE and send in chunks to TRS
    while (1){
        memset(buffer, (int)'\0', BUFFER_SIZE); // initializing the buffer with /0
        n = fread(buffer, 1, BUFFER_SIZE, file); // Reads, stores in buffer and returns the total number of elements successfully read
        ptr = buffer;
        if (n == -1){
            perror("Failed to read file");
            return 1;
        }
        else if (n == 0) break; // Leave the cycle when there's no more file data to read and send
        nleft = n;
        while(nleft > 0) {
            if (nleft < BUFFER_SIZE){
                strcat(buffer, "\n");
                nleft++;
            }
            nwritten = write(fd, ptr, nleft);
            if (nwritten == -1){
                perror("Failed to write file to TRS");
                return 1;
            }
            nleft -= nwritten;
            ptr += nwritten;
        }
        if (n < BUFFER_SIZE) break;
    }

    printf("Sent file %s\n", fileName);

    /**************** RECEIVING THE FILE FROM TRS *****************/

    memset(buffer, (int)'\0', BUFFER_SIZE); // initializing the buffer with /0
    ptr = buffer;
    nleft = BUFFER_SIZE;


    // First chunk received consists of request code, type, file name, file size and some data
    while(nleft > 0){
        nread = read(fd, ptr, nleft);
        if (nread == -1){
            perror("Failed to read file from TRS");
            return 1;
        }
        else if (nread == 0) break; // Closed by TRS
        nleft -= nread;
        ptr += nread;
    }

    printf("%s\n", buffer );

    token = strtok(buffer, SEPARATOR); // TRR
    if (!token){
        perror("No answer from TRS");
        return 1;
    }
    else if (strcmp(token, "TRR")){
        perror("Bad answer from TRS");
        return 1;
    }

    token = strtok(NULL, SEPARATOR); // error message if present, if not, ditch the type
    if (!token){
        perror("No error code provided from TRS");
        return 1;
    }
    else if (!strcmp(token, "NTA")){
        perror("NTA: No translation from TRS");
        return 1;
    }
    else if (!strcmp(token, "ERR")){
        perror("ERR: Bad request to TRS");
        return 1;
    }

    token = strtok(NULL, SEPARATOR); // get the file name
    if (!token){
        perror("No file name provided from TRS");
        return 1;
    }
    strcpy(fileName, token); // store file name

    token = strtok(NULL, SEPARATOR); // get file size
    if (!token){
        perror("No file size provided from TRS");
        return 1;
    }
    strcpy(fileLengthStr, token); // store file size
    fileLength = atoi(token); // file size as int

    /*token = strtok(NULL, SEPARATOR); // get the file data
    if (!token){
        perror("No file data provided from TRS");
        return 1;
    }*/


    indexData = strlen(request) + strlen(type) + strlen(fileName) + strlen(fileLengthStr) + 3;
    sizeData = BUFFER_SIZE - indexData;
    fileLength -= sizeData; // used in next read cycle, take the bytes already written to file

    file = fopen(fileName, "wb");
    fwrite(&buffer[indexData], 1, sizeData, file);


    // Now we read the rest of the file data in chunks of BUFFER_SIZE

    memset(buffer, (int)'\0', BUFFER_SIZE); // initializing the buffer with /0
    ptr = buffer;
    nleft = BUFFER_SIZE;
    while (1){
        memset(buffer, (int)'\0', BUFFER_SIZE); // initializing the buffer with /0
        ptr = buffer;
        nleft = BUFFER_SIZE;
        while(nleft > 0) {
            nread = read(fd, ptr, nleft);
            //printf("%d\n", nread);
            if (nread == -1){
                perror("Failed to read file from TRS");
                return 1;
            }
            else if (nread == 0) break; // Closed by TRS

            if (nread > fileLength){    // if TRS sent more bytes than the size of file, discard exceeding bytes
                nread = fileLength;
            }
            fileLength -= nread;

            n = fwrite(buffer, 1, nread, file); // Writes from buffer and returns the total number of elements successfully written

            if (n == -1){
                perror("Failed to write file");
                return 1;
            }
            else if (n < nread){
                perror("Failed to write all file data\n");
                return 1;
            }
            nleft -= nread;
            ptr += nread;
        }
        if (nread == 0) break; // Closed by TRS
    }

    // Check if the file size written matches the file size announced by TRS
    fseek(file, 0, SEEK_END); // Seek to the end of the file
    fileLength = ftell(file); // Get the size from the end position
    rewind(file);             // Go back to the start of the file

    printf("Received file %s\n%d Bytes\n", fileName, fileLength);

    fclose(file);
    return 0;
    }
