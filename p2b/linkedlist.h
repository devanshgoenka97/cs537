// Copyright [2022] <Devansh Goenka>

#ifndef __LINKEDLIST__
#define __LINKEDLIST__

struct node;

struct node* create();
struct node* find(char*);

int add(char*, char**);
int del(char*);

void printall();
void print(char *);
void printnode(struct node *);
void freeall();

#endif
