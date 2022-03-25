#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include "mapreduce.h"
#include "hashmap.h"

// structure definitions

typedef struct __node_t {
    struct __node_t* next;
    char* key;
    char* value;
} node_t;

typedef struct __list_t {
    pthread_mutex_t lock;
    int partition_number;
    node_t* head;
    node_t* current;
} list_t;

// global variables accessible to all threads
sem_t mapper_sem;
int partitions;
list_t** lists;
Mapper mapper;
Reducer reducer;
Partitioner partitioner;

// create a new node in the list -- method is thread safe
node_t* create(char* key, char* value) {
    node_t* new_node = (node_t *) malloc(sizeof(node_t));

    // malloc() failure
    assert(new_node != NULL);

    // initialize all fields
    new_node->next = NULL;
    new_node->key = strdup(key);
    new_node->value = strdup(value);

    return new_node;
}

// adds a node to a list -- method is thread safe
void add_to_list(list_t* list, char* key, char* value) {
    node_t* new_node = create(key, value);
    pthread_mutex_lock(list->lock);
    new_node->next = list->head;
    list->head = new_node;
    pthread_mutex_unlock(list->lock);
}

// deallocates an individual node in the list -- method is thread safe
void freenode(node_t* t) {
    // Freeing up indiviudal elements
    free(t->key);
    free(t->value);
    free(t);
}

// deallocates the entire list -- method is thread safe
void freeall(list_t* list){
    pthread_mutex_lock(list->lock);
    node_t* t = list->head;
    while (t != NULL) {
        struct node* next = t->next;
        freenode(t);
        t = next;
    }
    pthread_mutex_unlock(list->lock);
}

char* get_func(char* key, int partition_number) {

}

int cmp(const void* a, const void* b) {
    char* str1 = (*(struct kv **)a)->key;
    char* str2 = (*(struct kv **)b)->key;
    return strcmp(str1, str2);
}

void MR_Emit(char* key, char* value)
{
    int partition_num = (*partitioner)(key);
    add_to_list(lists[partition_num], key, value);
    return;
}

// default hashing routine using the dbj2 hash
unsigned long MR_DefaultHashPartition(char* key, int num_partitions) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++) != '\0')
        hash = hash * 33 + c;
    return hash % num_partitions;
}

// initializing global data structure
void init_lists(int num_reducers) {
    // creating as many lists as many reducers to assign each reducer one partition
    lists = (list_t **) malloc(num_reducers * sizeof(list_t));

    // checking malloc() failure
    assert(lists != NULL);

    // Initializing all lists with empty values and the lock as well
    for (int i = 0; i < num_reducers; i++) {
        lists[i]->head = NULL;
        lists[i]->current = NULL;
        lists[i]->partition_number = i;
        lists[i]->lock = PTHREAD_MUTEX_INITIALIZER;
    }
}

// multi-threaded map -- each map calls this, but only num_mappers go through at a time
void* __map_(void* arg) {
    char* filename = (char *) arg;

    // allowing num_mappers through, others wait
    sem_wait(&mapper_sem);
    // map the file
    (*mapper)(filename);
    // release the semaphore
    sem_post(&mapper_sem);

    pthread_exit(NULL);
}

// multi-threaded reduce -- each reducer gets a partition each
void* __reduce_(void* arg) {
    int partition_num = * (int *) arg;
    // call reduce for each key in the partition
    node_t* t;
    t = lists[partition_num]->head;
    while(t != NULL) {
        (*reducer)(t->key, get_func, partition_num);
        t = t->next;
    }
    // freeing the arg passed
    free(arg);
    pthread_exit(NULL);
}

void MR_Run(int argc, char *argv[], Mapper map, int num_mappers,
	    Reducer reduce, int num_reducers, Partitioner partition)
{
    mapper = map;
    reducer = reducer;
    partitioner = partition == NULL ? MR_DefaultHashPartition : partition;

    // initializing the semaphore
    sem_init(&mapper_sem, 0, num_mappers);

    // having as many partitions as reducers to give one reducer each partition
    partitions = num_reducers;
    init_lists(num_reducers);

    // creating mapper thread pool, semaphore will take care of num_mappers constraint
    pthread_t* mapper_threads = (pthread_t*) malloc(sizeof(pthread_t) * (argc - 1));

    // checking for malloc() failure
    assert(mapper_threads != NULL);

    int i;
    for (i = 1; i < argc; i++) {
        // call pthread_map
        if (pthread_create(&mapper_threads[i-1], NULL, &__map_, (void *) argv[i]) != 0) {
            // failure by pthread create
        }
    }

    // wait for mappers to finish
    for (int i = 0; i < argc - 1; i++) {
        pthread_join(mapper_threads[i], NULL);  
    }

    // add sorting routine

    // creating reducer thread pool to perform the reduction
    pthread_t* reducers_threads = (pthread_t*) malloc(sizeof(pthread_t) * num_reducers);

    // checking for malloc() failure
    assert(reducers_threads != NULL);

    for (int i = 0; i < num_reducers; i++) {
        // passing argument i as parititon number
        int* arg = (int *) malloc(sizeof(int));
        *arg = i;

        // call pthread_reduce
        if (pthread_create(&reducers_threads[i], NULL, &__reduce_, (void *) arg) != 0) {
            // failure by pthread create
        }
    }

    // wait for reducers to finish
    for (int i = 0; i < num_reducers; i++) {
        pthread_join(reducers_threads[i], NULL);  
    }

    sem_destroy(&mapper_sem);
}
