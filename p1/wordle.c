// Copyright [2022] <Devansh Goenka>

#include<stdio.h>
#include<stdlib.h>
#include<getopt.h>
#include<ctype.h>
#include<string.h>

#define BUFFERSIZE 255

int main(int argc, char** argv) {
    // Wordle needs 2 arguments the file for the dictionary and the input string
    if (argc != 3) {
        printf("wordle: invalid number of args\n");
        exit(1);
    }

    // Initialising the file pointers
    char* file_name = argv[1];
    char* blacklist = argv[2];

    FILE *fp = fopen(file_name, "r");
    if (fp == NULL) {
        printf("wordle: cannot open file\n");
        exit(1);
    }

    // For all ASCII characters, constructing the banned lookup map
    int map[128] = {0};
    for (int i = 0; blacklist[i] != '\0'; i++) {
        map[static_cast<int>(blacklist[i])] = 1;
    }

    char buffer[BUFFERSIZE];

    while (fgets(buffer, BUFFERSIZE, fp) != NULL) {
        // Removing the extra \n added by fgets() at the end of the buffer
        buffer[strcspn(buffer, "\n")] = '\0';

        // If the buffer length is not 5, then skip this line
        if (strlen(buffer) != 5) {
            continue;
        }

        // Initializing counter to 0 to indicate no banned characters
        int banned = 0;
        for (int i = 0; buffer[i] != '\0'; i++) {
            // The current character is banned, set the counter to 1 and break
            if (map[static_cast<int>(buffer[i])] > 0) {
                banned = 1;
                break;
            }
        }

        // Counter was not set, meaning all characters of this string are valid
        if (!banned) {
            printf("%s\n", buffer);
        }
    }

    fclose(fp);

    return 0;
}
