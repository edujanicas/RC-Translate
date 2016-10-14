#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define BUFFER_SIZE 512
#define min(A, B) ((A) < (B)?(A):(B))

void continueReading(char* buffer, char* fileName, int* doRead, int* doWrite, int* leftInFile);
void continueWriting(char* response, char* fileName, int* doWrite, int* leftInFile, int* firstExec);

void trsCore(char* buffer, char* reply ,char* language, char* fileName,
	int* doRead, int* doWrite, int* firstExec, int* leftInFile) {

	char *instruction, *word;
	char *line = NULL;
	char *brkt, *brkb;
	char numberOfWordsStr[2];
	char textFile[32];
	int numberOfWords, n, count;
	FILE *translation = NULL;
	size_t len = 0;
	ssize_t read;
	char fileSizeSTR[32];
	FILE* file;

	if (*doRead) {
		continueReading(buffer, fileName, doRead, doWrite, leftInFile);
		return;
	}
	if (*doWrite) {
		continueWriting(reply, fileName, doWrite, leftInFile, firstExec);
		return;
	}

	strcpy(reply, "ERR\n\0");

	instruction = strtok_r(buffer, " \n", &brkt);

	if (!strcmp(instruction, "TRQ")) {
		// The user requests the TRS to translate the provided text words (t) or image file (f).
		instruction = strtok_r(NULL, " \n", &brkt);

		if (!strcmp(instruction, "t")) {
			// TRQ t N W1 W2 ... WN
			// In the first case (t), N specifies the number of words that are being sent.
			// W1 W2 ... WN are strings specifying each of the N words, separated by spaces.
			// It can be assumed that each word (Wn) contains no more than 30 characters.
			// No more than 10 words are sent in each request instruction.


			strcpy(fileName, "text_translation-\0");
			strcat(fileName, language);
			strcat(fileName, ".txt\0");
			printf("%s\n", fileName);
			translation = fopen(fileName, "r");
			if (translation == NULL) {
				perror("Error opening text_translation.txt");
				strcpy(reply, "TRR ERR\n\0");
				return;
			}

			strcpy(reply, "TRR t ");
			strcpy(numberOfWordsStr, strtok_r(NULL, " \n", &brkt));
			numberOfWords = atoi(numberOfWordsStr);
			strcat(reply, numberOfWordsStr);

			for(n = 0; n < numberOfWords; n++) {

				count = 0;
				instruction = strtok_r(NULL, " \n", &brkt); // instruction retains the word to be translated

				while ((read = getline(&line, &len, translation)) != -1) {
					word = strtok_r(line, " \n", &brkb); // Then get the word
					if (!strcmp(instruction, word)) { // If it's the required one
						word = strtok_r(NULL, " \n", &brkb);
						strcat(reply, " "); // Append a whitespace between words
						strcat(reply, word);
						count++;
						break;
					}
				}
				rewind(translation); // Position the stream at the beggining of the file

				if (count == 0) {
					strcpy(reply, "TRR NTA");
				}
			}
			strcat(reply, "\n");
		}
	else if (!strcmp(instruction, "f")) {
		strcpy(fileName, strtok_r(NULL, " \n", &brkt));  // get the file name to translate
		continueReading(buffer, fileName, doRead, doWrite, leftInFile);

		// Answer -------------------------------
		*firstExec = 1;
		strcpy(textFile, "file_translation-\0");
		strcat(textFile, language);
		strcat(textFile, ".txt\0");
		translation = fopen(textFile, "r");

		if (translation == NULL) {
			perror("Error opening file_translation.txt");
			strcpy(reply, "TRR ERR\n\0");
			return;
		}
		strcpy(reply, "TRR f ");

		count = 0;

		while ((read = getline(&line, &len, translation)) != -1) {
			word = strtok_r(line, " \n", &brkb); // Then get the file
			if (!strcmp(fileName, word)) { // If it's the required one
				word = strtok_r(NULL, " \n", &brkb);
				strcat(reply, word);
				strcat(reply, " ");
				count++;
				break;
			}
		}
		rewind(translation); // Position the stream at the beggining of the file

		file = fopen(word, "rb");
		fseek(file, 0, SEEK_END); // Seek to the end of the file
		sprintf(fileSizeSTR, "%ld", ftell(file));
		fclose(file);
		strcat(reply, fileSizeSTR);
		strcat(reply, " ");

		if (count == 0) {
			strcpy(reply, "TRR NTA");
		}

	}
}
fclose(translation);
if (line) free(line);
}

void continueReading(char* buffer, char* fileName, int* doRead, int* doWrite, int* leftInFile) {
	char fileSizeSTR[32];
	int indexData, sizeData;
	FILE* file;

	file = fopen(fileName, "ab");

	if (!(*doRead)){
		indexData = strlen(fileName) + 7;
		strcpy(fileSizeSTR, strtok(&buffer[indexData], " \n"));
		*leftInFile = atoi(fileSizeSTR);
		indexData += strlen(fileSizeSTR) + 1;
		sizeData = BUFFER_SIZE - indexData;
		fwrite(&buffer[indexData], 1, sizeData, file);
		*doRead = 1;
		*leftInFile -= sizeData;
	}
	else {
		fwrite(buffer, 1, min(BUFFER_SIZE, *leftInFile), file);
		if (*leftInFile <= BUFFER_SIZE) {
			*doRead = 0;
			*doWrite = 1;
		}
		*leftInFile -= BUFFER_SIZE;
	}
	fclose(file);
}

void continueWriting(char* response, char* fileName, int* doWrite, int* leftInFile, int* firstExec) {

	FILE* file;

	if (*firstExec) {
		sscanf(&response[6], "%s" "%d", fileName, leftInFile);
		memset(response, (int)'\0', BUFFER_SIZE); // initializing the buffer with /0
		file = fopen(fileName, "rb");
		fread(response, 1, BUFFER_SIZE, file);
		*firstExec = 0;
	}
	else {
		memset(response, (int)'\0', BUFFER_SIZE); // initializing the buffer with /0
		file = fopen(fileName, "rb");

		fseek(file, -(*leftInFile), SEEK_END); // Seek to the end of the file
		fread(response, 1, min(BUFFER_SIZE, *leftInFile), file);

		if (*leftInFile <= BUFFER_SIZE) {
			*doWrite = 0;
			strcpy(&response[*leftInFile], "\n");
		}
	}
	*leftInFile -= BUFFER_SIZE;

	fclose(file);
}
