// Copyright [2022] <Devansh Goenka>

#include<stdlib.h>
#include<stdio.h>

#include "linkedlist.h"
#include "node.h"

struct node *HEAD = NULL;

struct node* create(char* name, char** args) {
    struct node* new_node = (struct node *) malloc(sizeof(struct node));

    if (new_node == NULL) {
        // malloc() failed
        return NULL;
    }

    new_node->next = NULL;
    new_node->name = strdup(name);
    for (int i = 0; i < 100; i++) { 
        new_node->args[i] = NULL;
    } 
    
    int j = 0;
    while(args[j] != NULL) {
        new_node->args[j] = strdup(args[j]);
        j++;
    }
    new_node->args[j] = NULL;

    return new_node;
}

int add(char *name, char **args) {
    if (HEAD == NULL) {
        HEAD = create(name, args);
        // in case malloc() fails
        if (HEAD == NULL)
            return -1;
    }
    else {
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

struct node* find(char *name) {
    struct node* temp = HEAD;
    struct node* p = NULL;

    while(temp != NULL) {
        if (strcmp(name, temp->name) == 0) {
            p = temp;
            break;
        }
        temp = temp->next;
    }

    return p;
}

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
    fflush(stdout);
}

void print(char *name) {
    struct node* p = find(name);
    if (p == NULL) 
        return;
    
    printnode(p);
}

void printall() {
    struct node *temp = HEAD;
    while(temp != NULL) {
        printnode(temp);
        temp = temp->next;
    }
}

int del(char *name) {
    struct node* temp = HEAD;
    struct node* prev = NULL;

    int found = 0;

    while(temp != NULL) {
        if (strcmp(name, temp->name) == 0) {
            struct node* t = temp;
            if (prev == NULL) {
                HEAD = temp->next;
            }
            else {
                prev->next = temp->next;
            }
            free(t);
            found = 1;
            break;
        }
        prev = temp;
        temp = temp->next;
    }

    return !found;
}

void freeall(){
    struct node* t = HEAD;
    while (t != NULL) {
        struct node* next = t->next;
        // Freeing up indiviudal elements
        free(t->name);
        for (int i = 0; i<100; i++) {
            free(t->args[i]);
        }
        free(t);
        t = next;
    }
}