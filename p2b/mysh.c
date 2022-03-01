// Copyright [2022] <Devansh Goenka>

#include<stdio.h>
#include<sys/wait.h>
#include<unistd.h>
#include<stdlib.h>
#include<ctype.h>
#include<string.h>
#include<fcntl.h>

#include "node.h"
#include "linkedlist.h"

// Buffer sizes
#define BUFFER_SIZE             512

// Debugging tools
#define DEBUG                   1
#define dprintf(...)            if (DEBUG) { printf(__VA_ARGS__); }       

// Bunch of error messages
#define ARG_ERR                 "Usage: mysh [batch-file]\n"
#define BATCH_FILE_ERR          "Error: Cannot open file %s.\n"
#define COMMAND_NOT_FOUND_ERR   "%s: Command not found.\n"
#define REDIRECTION_ERR         "Redirection misformatted.\n"
#define REDIRECT_FILE_ERR       "Cannot write to file %s.\n"

// Bunch of Constant strings
#define ALIAS                   "alias"
#define UNALIAS                 "unalias"
#define PROMPT                  "mysh> "
#define EXIT                    "exit"
#define DELIM                   " \t\r\n"
#define REDIRECT                ">"

int countargs(char **args);
void freemem(char *, char *, char *);

int main(int argc, char** argv) {

    // Initialising the batch file pointer
    char* file_name = NULL;
    int is_interactive = 1;

    if (argc > 2) {
        write(STDERR_FILENO, ARG_ERR, strlen(ARG_ERR));
        exit(1);
    }

    // if 2 arguments, it is a batch mode
    if (argc == 2) {
        is_interactive = 0;
        file_name = argv[1];
    }

    // Defaulting to the stdin FILE* for input
    FILE *fp = stdin;

    // Read from the batch file
    if (!is_interactive) {
        fp = fopen(file_name, "r");
        if (fp == NULL) {
            char buffer[BUFFER_SIZE];
            size_t length = snprintf(buffer, sizeof(buffer), BATCH_FILE_ERR, file_name);
            write(STDERR_FILENO, buffer, length);
            exit(1);
        }
    }

    char buffer[BUFFER_SIZE];

    // Print prompt
    if (is_interactive) {
        write(STDOUT_FILENO, PROMPT, strlen(PROMPT));
    }

    // Reading user input until Ctrl + D signal sent
    while (fgets(buffer, BUFFER_SIZE, fp) != NULL) {
        int execute_command = 1;

        // Print the command received in batch mode only
        if (!is_interactive) {
            write(STDOUT_FILENO, buffer, strlen(buffer));
        }

        // Removing the extra \n added by fgets()
        buffer[strcspn(buffer, "\n")] = 0;

        // If the buffer is just a new line, skip it
        if (strlen(buffer) == 0) {
            execute_command = 0;
        }

        // Variables to handle redirect scenario
        int is_redirect = 0;
        char* redirect_file = NULL;
        char* before_redir = NULL;
        char* after_redir = NULL;

        int k = 0;
        int pos = -1; 
        int count = 0;

        while (buffer[k] != 0) {
            if (buffer[k] == '>') {
                count++;
                pos = k;
            }
            k++;
        }

        // Handle case of more than 1 redirect
        if (count > 1) {
            // This should not happen, bad use of redirection
            write(STDERR_FILENO, REDIRECTION_ERR, strlen(REDIRECTION_ERR));
            execute_command = 0;
        }
        else if (count == 1) {
            is_redirect = 1;
            before_redir = (char *) malloc((pos + 1) * sizeof(char));
            after_redir = (char *) malloc((strlen(buffer) - pos - 1) * sizeof(char));

            strncpy(before_redir, &buffer[0], pos);
            strncpy(after_redir, &buffer[pos+1], strlen(buffer) - pos - 1);

            before_redir[pos] = '\0';
            after_redir[strlen(buffer) - pos - 1] = '\0';
        }

        char* command_tokenizer = buffer;

        if (is_redirect) {
            command_tokenizer = before_redir;

            // Extracting the file name out
            char* token = strtok(after_redir, DELIM);

            // Either empty token or more than one token
            if (token == NULL || strtok(NULL, DELIM) != NULL) {
                write(STDERR_FILENO, REDIRECTION_ERR, strlen(REDIRECTION_ERR));
                execute_command = 0;
            }
            else {
                redirect_file = strdup(token);
            }
        }

        // Using strtok() to tokenize the string with spaces, tabs
        char* token = strtok(command_tokenizer, DELIM);

        // Constructing argument array for the child command
        char *argv[100];
        int i = 0;

        while(token != NULL) {

            // Handle extra spaces and tabs
            if (strcmp(token, DELIM) == 0) {
                token = strtok(NULL, DELIM);
                continue;
            }

            // Construct argument array
            argv[i++] = token;
            token = strtok(NULL, DELIM);
        }

        // Place NULL at the end of the argument array
        argv[i] = NULL;

        // If all spaces in shell input
        if(i == 0) {
            execute_command = 0;
            if (is_redirect) {
                write(STDERR_FILENO, REDIRECTION_ERR, strlen(REDIRECTION_ERR));
            }
        }

        if (!execute_command) {
            // Printing the prompt again
            if (is_interactive) {
                write(STDOUT_FILENO, PROMPT, strlen(PROMPT));
            }

            freemem(before_redir, after_redir, redirect_file);

            continue;
        }

        if (strcmp(argv[0], EXIT) == 0) {
            freemem(before_redir, after_redir, redirect_file);
            break;
        }
        else if (strcmp(argv[0], ALIAS) == 0) {
            int count = countargs(argv);

            if (count >= 2) {
                add(argv[1], &argv[2]);
            }
            else if (count == 1) {
                print(argv[1]);
            }
            else {
                printall();
            }

            if (is_interactive) {
                write(STDOUT_FILENO, PROMPT, strlen(PROMPT));
            }

            freemem(before_redir, after_redir, redirect_file);
            continue;
        }
        else if (strcmp(argv[0], UNALIAS) == 0) {
            int count = countargs(argv);

            if (count > 1 || count == 0) {
                printf("unalias: Incorrect number of arguments.\n");
            }
            else {
                del(argv[1]);
            }

            if (is_interactive) {
                write(STDOUT_FILENO, PROMPT, strlen(PROMPT));
            }

            freemem(before_redir, after_redir, redirect_file);
            continue;
        }

        // check alias list to check if present
        struct node* t = find(argv[0]);
        if (t != NULL) {
            // Found an alias for the command, re-populate argv
            int i = 0;
            int j = 0;
            while (t->args[j] != NULL) {
                argv[i++] = t->args[j++];
            }
            argv[i] = NULL;
        }

        int pid = fork();
        int status;

        if (pid == 0) {
            // Inside child
            if (is_redirect) {
                close(STDOUT_FILENO);
                int opfile = open(redirect_file, O_TRUNC | O_RDWR | O_CREAT, 0666);
            
                if (opfile < 0) {
                    // Printing the correct error message to STDERR
                    char buffer[BUFFER_SIZE];
                    size_t length = snprintf(buffer, sizeof(buffer), REDIRECT_FILE_ERR, redirect_file);
                    write(STDERR_FILENO, buffer, length);
                    fclose(stdin);
                    exit(1);
                }
            }

            int rc_child = execv(argv[0], argv);

            if (rc_child != 0) {
                // Printing the correct error message to STDERR
                char buffer[BUFFER_SIZE];
                size_t length = snprintf(buffer, sizeof(buffer), COMMAND_NOT_FOUND_ERR, argv[0]);
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
        if (is_interactive) {
            write(STDOUT_FILENO, PROMPT, strlen(PROMPT));
        }

        freemem(before_redir, after_redir, redirect_file);
    }

    // close file pointer
    fclose(fp);

    // free alias list
    freeall();

    return 0;
}

void freemem(char *p1, char *p2, char *p3) {
    if (p1 != NULL)
        free(p1);
    if (p2 != NULL)
        free(p2);
    if (p3 != NULL)
        free(p3);

    p1 = NULL;
    p2 = NULL;
    p3 = NULL;
}

int countargs(char **args) {
    int count = 0;
    int j = 1;

    while (args[j] != NULL) {
        count++;
        j++;
    }

    return count;
}