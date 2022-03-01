// Copyright [2022] <Devansh Goenka>

#ifndef __NODE__
#define __NODE__

struct node {
    struct node *next;
    char *name;
    char *args[100];
};

#endif