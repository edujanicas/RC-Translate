#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void tcsCore(char* buffer, char* reply, int* nServers) {

	char *instruction, *language, tmp[2048];
	char *line = NULL;
	FILE *trsServers, *newTRSservers;
	size_t len = 0;
	ssize_t read;
	int n = 0;

	trsServers = fopen("languages.txt", "a+");
	if (trsServers == NULL) {
		perror("Error opening trsServers.txt");
		exit(EXIT_FAILURE);
	}

	strcpy(reply, "ERR\n\0");
	// Switch-Case cannot be used: C doens't support native string compairs
	if (!strcmp(buffer, "ULQ\n")) {

		/* If there was a query to the list of trsServers
		The response is of type ULR L1 L2 L3\n" */
		strcpy(reply, "ULR ");
		sprintf(tmp, "%d", *nServers);
		strcat(reply, tmp);

		/* The list of trsServers is on the trsServers.txt file.
		The file is of type L1 IP1 PORT1
		L2 IP2 PORT2, and so on
		We will only get the first word of each line */
		rewind(trsServers); // Back to the start of the file
		while ((read = getline(&line, &len, trsServers)) != -1) { // Search each line
			language = strtok(line, " \n"); // language is the first component of a line
			strcat(reply, " "); // Append a whitespace between trsServers
			strcat(reply, language);
		}
		strcat(reply, "\n"); // The reply[0]reply[0][0] must end with \n
	}

	else {

		instruction = strtok(buffer, " \n");

		if (!strcmp(instruction, "UNQ")) {

			/* If there was a query to request a language
			The response is of type UNR L1 IP1 PORT1\n" */

			strcpy(reply, "UNR");
			instruction = strtok(NULL, " \n");

			if (!instruction) { 		// Check if the instruction was sent
				fclose(trsServers);
				return;
			}
			n = atoi(instruction);		// If it was, check if it's a valid language
			if (n > *nServers) {
				fclose(trsServers);
				return;
			}

			/* The list of trsServers is on the trsServers.txt file.
			The file is of type L1 IP1 PORT1
			L2 IP2 PORT2, and so on
			We will only get the line of the requested language */
			rewind(trsServers);
			while ((read = getline(&line, &len, trsServers)) != -1) {
				strcpy(tmp, line); // We save the line in a temporary buffer
				language = strtok(line, " \n"); // Then get the language
				if (!strcmp(instruction, language)) { // If it's the required one
				language = strtok(NULL, " \n");
				strcat(reply, " "); // Append a whitespace between words
				strcat(reply, language);
				language = strtok(NULL, " \n");
				strcat(reply, " "); // Append a whitespace between words
				strcat(reply, language);
			}
		}
		strcat(reply, "\n"); // The reply[0]reply[0][0] must end with \n

		} else if (!strcmp(instruction, "SRG")) {

			/* If there was request to add a language SRG language IPTRS portTRS\n
			The response is of type SRR status\n" */

			language = strtok(NULL, " \n");
			strcpy(tmp, language);
			strcat(tmp, " ");
			strcat(tmp, strtok(NULL, " \n"));
			strcat(tmp, " ");
			strcat(tmp, strtok(NULL, " \n"));
			strcat(tmp, "\n");

			/* The list of trsServers is on the trsServers.txt file.
			The file is of type L1 IP1 PORT1
			L2 IP2 PORT2, and so on
			*/
			rewind(trsServers);
			while ((read = getline(&line, &len, trsServers)) != -1) {
				instruction = strtok(line, " \n"); // Then get the language
				if (!strcmp(language, instruction)) { // If it's already on the file, refuse
					strcpy(reply, "SRR NOK\n");
					fclose(trsServers);
					free(line);
					return;
				}
			}
			fwrite(tmp, strlen(tmp), 1, trsServers);
			rewind(trsServers);
			*nServers += 1;
			strcpy(reply, "SRR OK\n"); // The reply[0]reply[0][0] must end with \n
		} else if (!strcmp(instruction, "SUN")) {

			language = strtok(NULL, " \n");

			newTRSservers = fopen("languages~.txt", "w+");
			if (newTRSservers == NULL) {
				perror("Error opening languages~.txt");
				exit(EXIT_FAILURE);
			}

			rewind(trsServers);

			while ((read = getline(&line, &len, trsServers)) != -1) {
				strcpy(tmp, line); // Save the line in a temporary buffer
				instruction = strtok(line, " \n"); // Then get the language
 				if (!strcmp(language, instruction)) {
					*nServers -= 1;
				} else {
					fwrite(tmp, strlen(tmp), 1, newTRSservers);
					strcpy(reply, "SUR NOK\n");
				}
			}
			fclose(trsServers);
			remove("languages.txt");
			rename("languages~.txt", "languages.txt");
			fclose(newTRSservers);
			if (line) free(line);
			strcpy(reply, "SUR OK\n");
			return;
		}
	}

	fclose(trsServers);
	if (line) free(line);

}
