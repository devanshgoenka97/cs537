// Copyright [2022] <Devansh Goenka>

#include<stdio.h>
#include<sys/wait.h>
#include<unistd.h>
#include<stdlib.h>
#include<ctype.h>
#include<string.h>
#include<fcntl.h>

// Buffer sizes
#define BUFFER_SIZE             512

// Debugging tools
#define DEBUG                   1
#define dprintf(...)            if (DEBUG) { printf(__VA_ARGS__); }       

// Bunch of constant strings and error messages
#define ARG_ERR                 "Usage: mysh [batch-file]\n"
#define BATCH_FILE_ERR          "Error: Cannot open file %s.\n"
#define COMMAND_NOT_FOUND_ERR   "%s: Command not found.\n"
#define REDIRECTION_ERR         "Redirection misformatted.\n"
#define REDIRECT_FILE_ERR       "Cannot write to file %s.\n"

#define PROMPT                  "mysh> "
#define EXIT                    "exit"
#define DELIM                   " \t\r\n"
#define REDIRECT                ">"

struct node {
    struct node* next;
    char* name;
    char* args[100];
};

int main(int argc, char** argv) {

    // Initialising the batch file pointer
    char* file_name = NULL;
    int is_interactive = 1;

    if (argc > 2) {
        write(STDERR_FILENO, ARG_ERR, strlen(ARG_ERR));
        exit(1);
    }

    if (argc == 2) {
        is_interactive = 0;
        file_name = argv[1];
    }

    // Defaulting to the stdin FILE* for input
    FILE *fp = stdin;

    if (!is_interactive) {
        // Read from the batch file
        fp = fopen(file_name, "r");
        if (fp == NULL) {
            char buffer[BUFFER_SIZE];
            size_t length = snprintf(buffer, sizeof(buffer), BATCH_FILE_ERR, file_name);
            write(STDERR_FILENO, buffer, length);
            exit(1);
        }
    }

    char buffer[BUFFER_SIZE];
    struct node* HEAD = NULL;

    if (is_interactive) {
        write(STDOUT_FILENO, PROMPT, strlen(PROMPT));
    }

    // Reading user input until Ctrl + D signal sent
    while (fgets(buffer, BUFFER_SIZE, fp) != NULL) {
        int execute_command = 1;

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
            
            if (before_redir != NULL)
                free(before_redir);
            if (after_redir != NULL)
                free(after_redir);
            if (redirect_file != NULL)
                free(redirect_file);

            redirect_file = NULL;
            before_redir = NULL;
            after_redir = NULL;

            continue;
        }

        // Place NULL at the end of the argument array
        argv[i] = NULL;

        if (strcmp(argv[0], EXIT) == 0) {
            if (before_redir != NULL)
                free(before_redir);
            if (after_redir != NULL)
                free(after_redir);
            if (redirect_file != NULL)
                free(redirect_file);

            redirect_file = NULL;
            before_redir = NULL;
            after_redir = NULL;

            break;
        }
        else if (strcmp(argv[0], "alias") == 0) {
            int count = 0;
            int j = 1;

            while (argv[j] != NULL) {
                count++;
                j++;
            }

            if (count >= 2) {
                if (HEAD == NULL) {
                    HEAD = (struct node *) malloc(sizeof(struct node));
                    HEAD->next = NULL;
                }
                else {
                    struct node* temp = HEAD;
                    struct node* prev = NULL;
                    while(temp != NULL) {
                        if (strcmp(argv[1], temp->name) == 0) {
                            if (prev == NULL) {
                                free(HEAD);
                                HEAD  = NULL;
                                break;
                            }
                            struct node* f = temp;
                            prev->next = temp->next;
                            free(f);
                            break;
                        }
                        prev = temp;
                        temp = temp->next;
                    }
                    struct node* new_node = (struct node *) malloc(sizeof(struct node));
                    new_node->next = HEAD;
                    HEAD = new_node;
                }

                HEAD->name = strdup(argv[1]);
                for (int k = 0; k < count - 1; k++) {
                    HEAD->args[k] = strdup(argv[k+2]);
                }

                HEAD->args[k+1] = NULL;
            }
            else if (count == 1) {
                struct node* temp = HEAD;
                while(temp != NULL) {
                    if (strcmp(argv[1], temp->name) == 0) {
                        printf("%s", temp->name);
                        int k = 0;
                        while(temp->args[k] != NULL) {
                            printf(" %s", temp->args[k]);
                            k++;    
                        }
                        printf("\n");
                        break;
                    }
                    temp = temp->next;
                }
            }
            else {
                struct node* temp = HEAD;
                while(temp != NULL) {
                    printf("%s", temp->name);
                    int k = 0;
                    while (temp->args[k] != NULL) {
                        printf(" %s", temp->args[k]);
                        k++;    
                    }
                    printf("\n");
                    temp = temp->next;
                }
            }

            fflush(stdout);

            if (is_interactive) {
                write(STDOUT_FILENO, PROMPT, strlen(PROMPT));
            }

            continue;
        }
        else if (strcmp(argv[0], "unalias") == 0) {
            int count = 0;
            int j = 1;

            while (argv[j] != NULL) {
                count++;
                j++;
            }

            if (count > 1 || count == 0) {
                printf("unalias: Incorrect number of arguments.\n");
            }
            else {
                struct node* temp = HEAD;
                struct node* prev = NULL;
                while(temp != NULL) {
                    if (strcmp(argv[1], temp->name) == 0) {
                        if (prev == NULL)
                        {
                            HEAD = HEAD->next;
                            free(temp);
                            temp = NULL;
                        }
                        else {
                            prev->next = temp->next;
                            free(temp);
                            temp = NULL;
                        }
                        break;
                    }
                    prev = temp;
                    temp = temp->next;
                }
            }

            if (is_interactive) {
                write(STDOUT_FILENO, PROMPT, strlen(PROMPT));
            }

            continue;
        }

        // Traverse aliased linked list to check if any alias is present
        struct node* t = HEAD;
        while (t != NULL) {
            if (strcmp(t->name, argv[0]) == 0) {
                // Found an alias for the command, re-populate argv
                int i = 0;
                int j = 0;
                while (t->args[j] != NULL) {
                    argv[i++] = t->args[j++];
                }
                argv[i] = NULL;

                break;
            }
            t = t->next;
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

        if (before_redir != NULL)
            free(before_redir);
        if (after_redir != NULL)
            free(after_redir);
        if (redirect_file != NULL)
            free(redirect_file);
        before_redir = NULL;
        redirect_file = NULL;
        after_redir = NULL;
    }

    // Exiting shell, free alias list

    struct node *t  = HEAD;
    while (t != NULL) {
        struct node* p = t->next;
        free(t);
        t = p;
    }

    return 0;
}