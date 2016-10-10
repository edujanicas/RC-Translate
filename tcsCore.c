#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char* tcsCore(char* buffer, int* nServers) {

    char *instruction, *language, tmp[2048], reply[2048] = "ERR\n\0";
    char *line = NULL;

    FILE *trsServers;
    size_t len = 0;
    ssize_t read;

    trsServers = fopen("languages.txt", "a+");
    if (trsServers == NULL) {
        perror("Error opening trsServers.txt");
        exit(EXIT_FAILURE);
    }

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
          rewind(trsServers);
        while ((read = getline(&line, &len, trsServers)) != -1) {
            language = strtok(line, " \n"); // language is the first component of a line
            strcat(reply, " "); // Append a whitespace between trsServers
            strcat(reply, language);
        }
        strcat(reply, "\n"); // The reply must end with \n
    }

    else {

      instruction = strtok(buffer, " \n");

      if (!strcmp(instruction, "UNQ")) {

          /* If there was a query to request a language
            The response is of type UNR L1 IP1 PORT1\n" */

          strcpy(reply, "UNR");
          instruction = strtok(NULL, " \n");

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
          strcat(reply, "\n"); // The reply must end with \n

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
          fwrite(tmp, strlen(tmp), 1, trsServers);
          rewind(trsServers);

          (*nServers)++;
          strcpy(reply, "SRR OK\n"); // The reply must end with \n
      }
    }

    fclose(trsServers);
    if (line) free(line);

    return reply;
}
