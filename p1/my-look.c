#include<stdio.h>
#include<stdlib.h>
#include<getopt.h>
#include<ctype.h>
#include<string.h>

#define BUFFERSIZE 255

int main(int argc, char** argv)
{
    // Setting getopt()'s error to 0, to disable printing the default error
    opterr = 0;

    // Initialising the file pointers
    char* file_name = NULL;
    int is_stdin = 1;

    // Reading command line arguments
    int opt;
    while ((opt = getopt(argc, argv, "f:Vh")) != -1)
    {
        switch (opt)
        {
            case 'h':
                printf("my-look: usage 'my-look search-term' from stdin. use -f to specify file to read from, and -V for version information\n");
                return 0;
            case 'V':
                printf("my-look from CS537 Spring 2022\n");
                return 0;
            case 'f':
                is_stdin = 0;
                file_name = optarg;
                break;
            default:
                printf("my-look: invalid command line\n");
                exit(1);
        }
    }

    // Search string is the only required argument in the utility
    if (optind >= argc)
    {
        printf("my-look: search string not found, use -h on how to use\n");
        exit(1);
    }

    char* search = argv[optind];
    int size_search = strlen(search);

    // Defaulting to the stdin FILE* for input
    FILE *fp = stdin;

    if (!is_stdin)
    {
        // The -f flag was specified so read from file instead
        fp = fopen(file_name, "r");
        if (fp == NULL)
        {
            printf("my-look: cannot open file\n");
            exit(1);
        }
    }

    char buffer[BUFFERSIZE];

    // TODO: Online vs offline checking with STDIN
    while (fgets(buffer, BUFFERSIZE, fp) != NULL)
    {
        // Removing the extra \n added by fgets() at the end of the buffer
        buffer[strcspn(buffer, "\n")] = 0;

        char cmp_string[BUFFERSIZE];
        int i = 0;
        int j = 0;
        while (buffer[i] != '\0')
        {
            // Only considering alphanumeric characters from the input string
            if (isalnum(buffer[i]))
            {
                cmp_string[j++] = buffer[i];
            }
            i++;
        }

        // If the buffer only consists of the new line which was removed earlier, then skip this line
        if(strlen(buffer) == 0)
        {
            continue;
        }

        // Comparing the the sizeof(search) bytes of the search string with the cleaned up input string
        if (strncasecmp(search, cmp_string, size_search) == 0)
        {
            printf("%s\n", buffer);
        }
    }

    return 0;
}