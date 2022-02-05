#include<stdio.h>
#include<stdlib.h>
#include<getopt.h>
#include<ctype.h>
#include<string.h>

#define BUFFERSIZE 255

int main(int argc, char** argv)
{
    // Search string is the only required argument in the utility
    if (argc != 3)
    {
        printf("wordle: invalid number of args\n");
        exit(1);
    }

    // Initialising the file pointers
    char* file_name = argv[1];
    char* blacklist = argv[2];

    FILE *fp = fopen(file_name, "r");
    if (fp == NULL)
    {
        printf("wordle: cannot open file\n");
        exit(1);
    }

    int map[128] = {0};
    
    // Constructing the blacklist lookup map
    for(int i = 0; blacklist[i]!='\0'; i++)
    {
        map[(int)blacklist[i]] = 1;
    }

    char buffer[BUFFERSIZE];

    while (fgets(buffer, BUFFERSIZE, fp) != NULL)
    {
        // Removing the extra \n added by fgets() at the end of the buffer
        buffer[strcspn(buffer, "\n")] = '\0';

        // If the buffer length is not 5, then skip this line, as we only consider strings of length 5
        if(strlen(buffer) != 5)
        {
            continue;
        }

        int banned = 0;
        for(int i = 0;buffer[i] != '\0'; i++)
        {
            if(map[(int)buffer[i]] > 0)
            {
                banned = 1;
                break;
            }
        }

        if(!banned)
        {
            printf("%s\n", buffer);
        }
    }

    fclose(fp);

    return 0;
}