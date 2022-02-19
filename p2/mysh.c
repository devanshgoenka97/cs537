// Copyright [2022] <Devansh Goenka>

#include<stdio.h>
#include<sys/wait.h>
#include<unistd.h>
#include<stdlib.h>
#include<ctype.h>
#include<string.h>
#include<fcntl.h>

#define BUFFERSIZE 512

int main(int argc, char** argv) {

    // Initialising the batch file pointer
    char* file_name = NULL;
    int is_interactive = 1;

    if(argc == 2){
        is_interactive = 0;
        file_name = argv[1];
    }

    // Defaulting to the stdin FILE* for input
    FILE *fp = stdin;

    if (!is_interactive) {
        // Read from the batch file
        fp = fopen(file_name, "r");
        if (fp == NULL) {
            printf("mysh: cannot open file\n");
            exit(1);
        }
    }

    char buffer[BUFFERSIZE];

    // Printing the prompt to be displayed
    const char* prompt = "mysh> ";
    write(STDOUT_FILENO, prompt, strlen(prompt));

    // Reading user input until Ctrl + D signal sent
    while (fgets(buffer, BUFFERSIZE, fp) != NULL) {

        // Removing the extra \n added by fgets()
        buffer[strcspn(buffer, "\n")] = 0;

        // If the buffer is just a new line, skip it
        if (strlen(buffer) == 0) {
            write(STDOUT_FILENO, prompt, strlen(prompt));
            continue;
        }

        const char *delim = " ";
        const char *redirect = ">";

        // Using strtok() to tokenize the string with spaces
        char* token = strtok(buffer, delim);

        // Constructing argument array for the child command
        char *argv[100];
        int i = 0;

        // Variables to handle redirect scenario
        int is_redirect = 0;
        char* redirect_file = NULL;

        int execute_command = 1;

        while(token != NULL){

            // Handle extra spaces
            if (strcmp(token, delim) == 0){
                token = strtok(NULL, delim);
                continue;
            }

            // Handle redirect
            if (strcmp(token, redirect) == 0){
                // Next token should be the file name
                is_redirect = 1;
                redirect_file = strtok(NULL, delim);

                if (strtok(NULL, delim) != NULL){
                    // This should not happen, bad use of redirection
                    write(STDERR_FILENO, "Redirection misformatted.\n", strlen("Redirection misformatted.\n"));
                    execute_command = 0;
                    break;
                }

                break;
            }

            // Construct argument array
            argv[i++] = token;
            token = strtok(NULL, delim);
        }

        // If all spaces in shell input
        if(i == 0){
            execute_command = 0;
        }

        if (!execute_command) {
            // Printing the prompt again
            write(STDOUT_FILENO, prompt, strlen(prompt));
            continue;
        }

        // Place NULL at the end of the argument array
        argv[i] = NULL;

        int pid = fork();
        int status;

        if(pid == 0)
        {
            // Inside child
            if (is_redirect) {
                close(STDOUT_FILENO);
                int opfile = open(redirect_file, O_RDWR | O_CREAT, 0666);
            
                if (opfile < 0){
                    // Printing the correct error message to STDERR
                    char buffer[1024];
                    size_t length = snprintf(buffer, sizeof(buffer), "Cannot write to file %s.\n", redirect_file);
                    write(STDERR_FILENO, buffer, length);
                }
            }

            int rc_child = execv(argv[0], argv);

            if (rc_child != 0) {
                // Printing the correct error message to STDERR
                char buffer[1024];
                size_t length = snprintf(buffer, sizeof(buffer), "%s: Command not found.\n", argv[0]);
                write(STDERR_FILENO, buffer, length);
                fclose(stdin);
                exit(1);
            }
        }
        else {
            // Inside parent
            waitpid(pid, &status, 0);
        }

        // Printing the prompt again
        write(STDOUT_FILENO, prompt, strlen(prompt));
    }

    // Extra line to clean output to the original shell
    printf("\n");
    return 0;
}