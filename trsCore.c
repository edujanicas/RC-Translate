#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void trsCore(char* buffer, char* reply ,char* language) {

	char *instruction, *word;
	char *line = NULL;
	char *brkt, *brkb;
	char numberOfWordsStr[2];
	char fileName[32];
	int numberOfWords, n, count;

	FILE *translation = NULL;
	size_t len = 0;
	ssize_t read;

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
			count = 0;
			
			for(n = 0; n < numberOfWords; n++) {

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

	} else if (!strcmp(instruction, "f")) {

		translation = fopen("file_translation.txt", "r");
		if (translation == NULL) {
			perror("Error opening file_translation.txt");
			strcpy(reply, "TRR ERR\n\0");
			return;
		}
	}
}
fclose(translation);
if (line) free(line);
}
