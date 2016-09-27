#include <stdlib.h>
#include <string.h>

void printWords(char* buffer) {

char* word;

/* get the first word from the message, seperated by
* space character */
word = strtok(buffer, " \n");
printf("%s\n", word);
/* the following loop gets the rest of the words until the
* end of the message */
while ((word = strtok(NULL, " \n")) != NULL)
printf("%s\n", word);

}

char* tcsCore(char* buffer) {

    char *instruction, *language, reply[2048], tmp[2048];
    char EOS = '\n', separator = ' ', *line = NULL;
    FILE * languages;
    size_t len = 0;
    ssize_t read;

    languages = fopen("languages.txt", "r");
    if (languages == NULL) exit(EXIT_FAILURE);

    instruction = strtok(buffer, &separator);

    if (!strcmp(instruction, "ULQ")) {

        /* If there was a query to the list of languages
          The response is of type ULR L1 L2 L3\n" */

        strcpy(reply, "ULR");

        /* The list of languages is on the languages.txt file.
          The file is of type L1 IP1 PORT1
                              L2 IP2 PORT2, and so on
          We will only get the first word of each line */

        while ((read = getline(&line, &len, languages)) != -1) {
language = strtok(line, &separator);
          strcat(reply, " "); // Append a whitespace between languages
          strcat(reply, language);
        }

strcat(reply, "\n"); // The reply must end with \n

        return reply;

    }
    else if (!strcmp(instruction, "UNQ")) {

      /* If there was a query to request a language
          The response is of type UNR L1 IP1 PORT1\n" */

strcpy(reply, "UNR");
instruction = strtok(NULL, &separator);

        /* The list of languages is on the languages.txt file.
          The file is of type L1 IP1 PORT1
                              L2 IP2 PORT2, and so on
          We will only get the line of the requested language */

while ((read = getline(&line, &len, languages)) != -1) {

          strcpy(tmp, line); // We save the line in a temporary buffer

language = strtok(line, &separator); // Then get the language

          if (!strcmp(instruction, language)) { // If it's the required one
              strcat(reply, " ");
strcat(reply, tmp);
}
        }

strcat(reply, "\n"); // The reply must end with \n

      return reply;

    }

    fclose(languages);
    if (line) free(line);

    return "ERR\n\0";
}
