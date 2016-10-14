#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define BUFFER_SIZE 512
#define min(A, B) ((A) < (B)?(A):(B))

void continueReading(char* buffer, char* fileName, int* doRead, int* leftInFile);
void trsCore(char* buffer, char* reply ,char* language, char* fileName, int* doRead, int* doWrite, int* leftInFile) {

	char *instruction, *word;
	char *line = NULL;
	char *brkt, *brkb;
	char numberOfWordsStr[2];
	int numberOfWords, n, count;
	FILE *translation = NULL;
	size_t len = 0;
	ssize_t read;

	if (&doRead) continueReading(buffer, fileName, doRead, leftInFile);
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
		continueReading(buffer, fileName, doRead, leftInFile);
		// Answer
		strcpy(fileName, "file_translation-\0");
		strcat(fileName, language);
		strcat(fileName, ".txt\0");
		translation = fopen(fileName, "r");
		
		if (translation == NULL) {
			perror("Error opening text_translation.txt");
			strcpy(reply, "TRR ERR\n\0");
			return;
		}
		strcpy(reply, "TRR t ");

		count = 0;
		
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
}
fclose(translation);
if (line) free(line);
}

void continueReading(char* buffer, char* fileName, int* doRead, int* leftInFile) {
	char* token;
	char fileSizeSTR[32];
	int indexData, sizeData;
	FILE* file

	file = fopen(fileName, "wb");

	if (!doRead){
		indexData = strlen(fileName) + 7;
		strcpy(fileSizeSTR, strtok(&buffer[indexData]));
		&leftInFile = atoi(fileSizeSTR);
		indexData += strlen(fileSizeSTR) + 1;
		sizeData = BUFFER_SIZE - indexData;
		fwrite(&buffer[indexData], 1, sizeData, file);
		&doRead = 1;
		&leftInFile -= sizeData;
	}
	else {
		fwrite(buffer, min(BUFFER_SIZE, &leftInFile), file);
		if (&leftInFile <= BUFFER_SIZE) &doRead = 0;
		&leftInFile -= BUFFER_SIZE;
	}
	fclose(file);
}

