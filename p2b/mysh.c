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
#define ALIAS_FORBIDDEN         "alias: Too dangerous to alias that.\n"
#define UNLALIAS_ARGS           "unalias: Incorrect number of arguments.\n"

// Bunch of Constant strings
#define ALIAS                   "alias"
#define UNALIAS                 "unalias"
#define PROMPT                  "mysh> "
#define EXIT                    "exit"
#define DELIM                   " \t\r\n"
#define REDIRECT                ">"

// Function prorotypes for ease of use
int countargs(char **);
int tokenize(char *, char **);
int handleredirect(char *, char **, char **);
void execute(int, char *, char **);
void freemem(char *, char *, char *);

int main(int argc, char** argv) {

    // variables needed for the batch file pointer
    char* file_name = NULL;
    int is_interactive = 1;

    // print incorrect number of args to mysh
    if (argc > 2) {
        write(STDERR_FILENO, ARG_ERR, strlen(ARG_ERR));
        exit(1);
    }

    // if 2 arguments, it is batch mode
    if (argc == 2) {
        is_interactive = 0;
        file_name = argv[1];
    }

    // Defaulting to the stdin FILE* for input
    FILE *fp = stdin;

    // Read from the batch file
    if (!is_interactive) {
        fp = fopen(file_name, "r");

        // unable to read batch file
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
        // variable to decide if to execute or not later
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

        // check for redirect scenario
        int is_redirect = 0;
        char* redirect_file = NULL;
        char* command_tokenizer = strdup(buffer);

        // call redirect handler
        int redir = handleredirect(buffer, &redirect_file, &command_tokenizer);

        // successful case of redirection
        if (redir == 0) {
            is_redirect = 1;
        }
        // bad use of redirection, print to stderr and not execute command
        else if (redir == 1) {
            write(STDERR_FILENO, REDIRECTION_ERR, strlen(REDIRECTION_ERR));
            execute_command = 0;
        }

        // Constructing argument array for the child command
        char *argv[100];

        // tokenize and get the len of args array
        int len = tokenize(command_tokenizer, argv);

        // if no arguments populated, no command given
        if(len == 0) {
            execute_command = 0;

            // bad use of redirection, no command given
            if (is_redirect) {
                write(STDERR_FILENO, REDIRECTION_ERR, strlen(REDIRECTION_ERR));
            }
        }

        // deciding to not execute command
        if (!execute_command) {
            // Printing the prompt again
            if (is_interactive) {
                write(STDOUT_FILENO, PROMPT, strlen(PROMPT));
            }

            freemem(redirect_file, command_tokenizer, NULL);

            continue;
        }

        // built-in command : "exit"
        if (strcmp(argv[0], EXIT) == 0) {
            freemem(redirect_file, command_tokenizer, NULL);
            break;
        }
        // built-in command : "alias"
        else if (strcmp(argv[0], ALIAS) == 0) {
            int count = countargs(argv);

            // args >= 2, add node to alias list
            if (count >= 2) {
                // check for forbidden aliases

                if (strcmp(argv[1], ALIAS) == 0 || 
                    strcmp(argv[1], UNALIAS) == 0 || 
                    strcmp(argv[1], EXIT) == 0 )  {

                    write(STDERR_FILENO, ALIAS_FORBIDDEN, strlen(ALIAS_FORBIDDEN));
                }
                else {
                    add(argv[1], &argv[2]);
                }
            }
            // find and print the matching node
            else if (count == 1) {
                print(argv[1]);
            }
            // print the entire list
            else {
                printall();
            }

            if (is_interactive) {
                write(STDOUT_FILENO, PROMPT, strlen(PROMPT));
            }

            freemem(redirect_file, command_tokenizer, NULL);
            continue;
        }
        // built-in command : "unalias"
        else if (strcmp(argv[0], UNALIAS) == 0) {
            int count = countargs(argv);

            // only 1 arg expected
            if (count > 1 || count == 0) {
                write(STDERR_FILENO, UNLALIAS_ARGS, strlen(UNLALIAS_ARGS));
            }
            // remove node from alias list
            else {
                del(argv[1]);
            }

            if (is_interactive) {
                write(STDOUT_FILENO, PROMPT, strlen(PROMPT));
            }

            freemem(redirect_file, command_tokenizer, NULL);
            continue;
        }

        // check alias list to check if present
        struct node* t = find(argv[0]);

        // found an alias, re-populate argv
        if (t != NULL) {
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
            // Inside child, call execute
            execute(is_redirect, redirect_file, argv);
        }
        else {
            // Inside parent
            waitpid(pid, &status, 0);
        }

        // Printing the prompt again
        if (is_interactive) {
            write(STDOUT_FILENO, PROMPT, strlen(PROMPT));
        }

        freemem(redirect_file, command_tokenizer, NULL);
    }

    // close file pointer
    fclose(fp);

    // free alias list
    freeall();

    return 0;
}

// executes the given arguments using execv()
void execute(int is_redirect, char* redirect_file, char** argv) {
    // if redirection requested, handle fds
    if (is_redirect) {
        // close the stdout fd
        close(STDOUT_FILENO);

        // open the requested file
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

    // execute the command
    int rc = execv(argv[0], argv);

    // execv failed
    if (rc != 0) {
        // Printing the correct error message to STDERR
        char buffer[BUFFER_SIZE];
        size_t length = snprintf(buffer, sizeof(buffer), COMMAND_NOT_FOUND_ERR, argv[0]);
        write(STDERR_FILENO, buffer, length);

        // since stdin is already open for parent
        fclose(stdin);
        exit(1);
    }
}

// tokenizes the command and populates the "args" array
int tokenize(char *command_tokenizer, char **args) {
    // Using strtok() to tokenize the string with spaces, tabs
    char* token = strtok(command_tokenizer, DELIM);

    int i = 0;

    // while the tokens exist
    while(token != NULL) {

        // Handle extra spaces and tabs
        if (strcmp(token, DELIM) == 0) {
            token = strtok(NULL, DELIM);
            continue;
        }

        // Construct argument array
        args[i++] = token;
        token = strtok(NULL, DELIM);
    }

    // Place NULL at the end of the argument array
    args[i] = NULL;

    // Return the length of tokenized args
    return i;
}

// handles the redirect scenario and populates "redirect_file" if true
int handleredirect(char* buffer, char** redirect_file, char** command_tokenizer) {
    // variables needed for redirection
    char* before_redir = NULL;
    char* after_redir = NULL;
    int k = 0;
    int pos = -1; 
    int count = 0;
    int is_redirect = 0;

    // check if the redirect symbol appears in buffer
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
        return 1;
    }
    // valid case of redirect
    else if (count == 1) {
        is_redirect = 1;

        // separate input into before and after redirect symbol
        before_redir = (char *) malloc((pos + 1) * sizeof(char));
        after_redir = (char *) malloc((strlen(buffer) - pos - 1) * sizeof(char));

        // copy input to new memory malloc()'ed
        strncpy(before_redir, &buffer[0], pos);
        strncpy(after_redir, &buffer[pos+1], strlen(buffer) - pos - 1);

        // place end of string characters
        before_redir[pos] = '\0';
        after_redir[strlen(buffer) - pos - 1] = '\0';
    }

    if (is_redirect) {
        // free the populated command tokenizer
        free(*command_tokenizer);

        // only tokenize the commands before redir symbol;
        *command_tokenizer = strdup(before_redir);

        // Extracting the file name out
        char* token = strtok(after_redir, DELIM);

        // Either empty token or more than one token
        if (token == NULL || strtok(NULL, DELIM) != NULL) {
            return 1;
        }
        else {
            *redirect_file = strdup(token);
        }

        // free the redir pointers, it is already duplicated
        freemem(after_redir, before_redir, NULL);
        return 0;
    }

    // not a redirect scenario
    return -1;
}

// deallocates pointers
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

// utility to count the number of arguments
int countargs(char **args) {
    int count = 0;
    int j = 1;

    while (args[j] != NULL) {
        count++;
        j++;
    }

    return count;
}