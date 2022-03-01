// Copyright [2022] <Devansh Goenka>

#include<stdlib.h>
#include<string.h>
#include<stdio.h>

#include "linkedlist.h"
#include "node.h"

// stores the head of the linked list
struct node *HEAD = NULL;

// create a new node in the list
struct node* create(char* name, char** args) {
    struct node* new_node = (struct node *) malloc(sizeof(struct node));

    if (new_node == NULL) {
        // malloc() failed
        return NULL;
    }

    // initialize all fields
    new_node->next = NULL;
    new_node->name = strdup(name);
    for (int i = 0; i < 100; i++) { 
        new_node->args[i] = NULL;
    } 

    // populate all fields
    int j = 0;
    while(args[j] != NULL) {
        new_node->args[j] = strdup(args[j]);
        j++;
    }
    new_node->args[j] = NULL;

    return new_node;
}

// create a new node and add to the list
int add(char *name, char **args) {
    if (HEAD == NULL) {
        HEAD = create(name, args);
        // in case malloc() fails
        if (HEAD == NULL)
            return -1;
    }
    else {
        // if a node with same name exists, delete
        del(name);
        struct node* new_node = create(name, args);
        // in case malloc() fails
        if (new_node == NULL)
            return -1;
        new_node->next = HEAD;
        HEAD = new_node;
    }

    return 0;
}

// find a node in the list given the name
struct node* find(char *name) {
    struct node* temp = HEAD;
    struct node* p = NULL;

    while(temp != NULL) {
        // if name matches, return
        if (strcmp(name, temp->name) == 0) {
            p = temp;
            break;
        }
        temp = temp->next;
    }

    // if not found, p is NULL
    return p;
}

// print a specific node of the list to stdout
void printnode(struct node* t) {
    if (t == NULL)
        return;
    
    printf("%s", t->name);

    int k = 0;
    while(t->args[k] != NULL) {
        printf(" %s", t->args[k]);
        k++;    
    }
    printf("\n");

    // flush the buffer to stdout
    fflush(stdout);
}

// finds a node with given name and prints it
void print(char *name) {
    struct node* p = find(name);
    if (p == NULL) 
        return;
    
    printnode(p);
}

// prints the entire list to stdout
void printall() {
    struct node *temp = HEAD;

    while(temp != NULL) {
        printnode(temp);
        temp = temp->next;
    }
}

// deletes a node from the list
int del(char *name) {
    struct node* temp = HEAD;
    struct node* prev = NULL;

    int found = 0;

    while(temp != NULL) {
        // if the name matches, this is the node to del
        if (strcmp(name, temp->name) == 0) {
            struct node* t = temp;
            if (prev == NULL) {
                // head is the matching node, just increment
                HEAD = temp->next;
            }
            else {
                // link up the previous to next
                prev->next = temp->next;
            }
            // deallocate the node
            freenode(t);
            found = 1;
            break;
        }
        prev = temp;
        temp = temp->next;
    }

    return !found;
}

// deallocates an individual node in the list
void freenode(struct node *t) {
    // Freeing up indiviudal elements
    free(t->name);
    for (int i = 0; i<100; i++) {
        free(t->args[i]);
    }
    free(t);
}

// deallocates the entire list
void freeall(){
    struct node* t = HEAD;
    while (t != NULL) {
        struct node* next = t->next;
        freenode(t);
        t = next;
    }
}