#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include "mapreduce.h"
#include "hashmap.h"

// global structure definitions
typedef struct __node_t {
    struct __node_t* next;
    char* key;
    char* value;
} node_t;

typedef struct __list_t {
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
    // create a node
    node_t* new_node = create(key, value);

    // perform wait-free addition to head of list
    do {
        new_node->next = list->head;  // re-check to update head
      // perform compare and swap until it succeeds
    } while (!__sync_bool_compare_and_swap(&list->head, new_node->next, new_node));
}

// custom compare function for sorting list
int cmp(const void* a, const void* b) {
    node_t* first = *(node_t**)a;
    node_t* second = *(node_t**)b;
    // return the comparison between keys
    return strcmp(first->key, second->key);
}

// sort a linked list by using library defined qsort
void sort(node_t** head) {
    int size = 0;
    node_t *t = *head;

    // calculate size of list
    while (t != NULL) {
        t = t->next;
        size++;
    }

    // no need to sort empty or single element list
    if (size == 0 || size == 1) {
        return;
    }

    // convert list to array of pointers
    // size + 1 because last element is NULL
    node_t** arr = (node_t**) malloc(sizeof(node_t*) * (size+1));
    
    node_t** temp = arr;

    // traverse list to assign pointers contiguosly
    t = *head;
    while (t != NULL) {
        *temp = t;
        temp++;
        t = t->next;
    }
    *temp = NULL;

    // library qsort on array
    qsort(arr, size, sizeof(node_t**), cmp);

    // point the nodes in the sorted order
    for (int i = 0; i < size; ++i) {
        arr[i]->next = arr[i+1];
    }

    *head = *arr;

    // free the allocated space for sorting
    free(arr);
}

// deallocates an individual node in the list -- method is thread safe
void freenode(node_t* t) {
    // Freeing up indiviudal elements
    free(t->key);
    free(t->value);
    free(t);
}

// deallocates the entire list -- method is NOT thread safe
void freeall(node_t* head){
    node_t* t = head;
    while (t != NULL) {
        node_t* next = t->next;
        freenode(t);
        t = next;
    }
}

// initializing global data structure
void init_lists(int num_lists) {
    // creating as many lists as many reducers to assign each reducer one partition
    lists = (list_t **) malloc(num_lists * sizeof(list_t*));

    // checking malloc() failure
    assert(lists != NULL);

    // Initializing all lists with empty values and the lock as well
    for (int i = 0; i < num_lists; i++) {
        lists[i] = (list_t *) malloc(sizeof(list_t));

        // checking malloc() failure
        assert(lists[i] != NULL);

        lists[i]->head = NULL;
        lists[i]->current = NULL;
        lists[i]->partition_number = i;
    }
}

// deallocate the global variables
void free_lists(int num_lists) {
    // Freeing up all lists
    for (int i = 0; i < num_lists; i++) {
        freeall(lists[i]->head);
        free(lists[i]);
    }

    // once individual components are freed up, free the allocation
    free(lists);
}

// implementation of the getter func -- fetches one val for key and advances the current ptr
char* get_func(char* key, int partition_number) {
    list_t* list = lists[partition_number];
    node_t* current = list->current;

    if (current != NULL && strcmp(current->key, key) == 0) {
        char* currentval = current->value;
        list->current = list->current->next;
        current = current->next;
        return currentval;
    }

    return NULL;
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

// multi-threaded sort -- each sorter gets a partition
void* __sort_(void* arg) {
    int partition_num = *(int *) arg;

    // call merge sort for the list
    list_t* list = lists[partition_num];
    sort(&list->head);

    // freeing the arg passed
    free(arg);
    pthread_exit(NULL);
}

// multi-threaded reduce -- each reducer gets a partition each
void* __reduce_(void* arg) {
    int partition_num = *(int *) arg;

    // call reduce for each key in the partition
    list_t* list = lists[partition_num];
    list->current = list->head;
    node_t* t = list->head;

    while(t != NULL) {
        (*reducer)(t->key, get_func, partition_num);
        t = list->current;
    }

    // freeing the arg passed
    free(arg);
    pthread_exit(NULL);
}

// emits a key-value pair to the appropriate partitioned list
void MR_Emit(char* key, char* value)
{
    int partition_num = (*partitioner)(key, partitions);
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

// main function to be invoked for the library
void MR_Run(int argc, char *argv[], Mapper map, int num_mappers,
	    Reducer reduce, int num_reducers, Partitioner partition)
{
    mapper = map;
    reducer = reduce;
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

    free(mapper_threads);

    // creating sorted thread pool to perform the sorting on each partition
    pthread_t* sorter_threads = (pthread_t*) malloc(sizeof(pthread_t) * num_reducers);

    // checking for malloc() failure
    assert(sorter_threads != NULL);

    for (int i = 0; i < num_reducers; i++) {
        // passing argument i as parititon number
        int* arg = (int *) malloc(sizeof(int));
        *arg = i;

        // call pthread_reduce
        if (pthread_create(&sorter_threads[i], NULL, &__sort_, (void *) arg) != 0) {
            // failure by pthread create
        }
    }

    // wait for sorters to finish
    for (int i = 0; i < num_reducers; i++) {
        pthread_join(sorter_threads[i], NULL);  
    }

    free(sorter_threads);

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

    free(reducers_threads);

    sem_destroy(&mapper_sem);
    free_lists(num_reducers);
}
