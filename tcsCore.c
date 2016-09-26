#include <stdlib.h>
#include <string.h>

char* tcsCore(char* buffer) {

    char *instruction;
    char separator = ' ';
    char EOS = '\n';

    printf("On tcsCore\n");

    instruction = malloc(4 * sizeof(char));
    instruction = strtok(buffer, &separator);

    printf("%s\n", instruction);

    if (!strcmp(instruction, "ULQ\n")) {
        return "ULR\0";
    } else if (!strcmp(instruction, "UNQ ")) {
        return "UNR\0";
    }
    return "TESTE\0";
}
