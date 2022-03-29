#include "types.h"
#include "user.h"

#define PAGESIZE    4096
#define MAX_THREADS 64

typedef struct __thread_stacks_t {
  void* op;
  void* tstack;
  int inuse;
} thread_stacks_t;

// Used for keeping track of thread stacks to be freed
thread_stacks_t thread_stacks[MAX_THREADS];

int
thread_create(void (*fcn)(void*, void*), void* arg1, void* arg2)
{
  void* mem = malloc(PAGESIZE*2);

  uint stack;

  // align to page if not page aligned, will take care automatically
  stack = (uint)mem + (PAGESIZE - (uint)mem % PAGESIZE);

  int rc = clone(fcn, arg1, arg2, (void *)stack);

  if (rc != -1)
  {
    for (int i=0; i<MAX_THREADS; i++)
    {
      // Using an empty slot
      if(thread_stacks[i].inuse == 0)
      {
        // Storing the original malloc'ed pointer for freeing later
        thread_stacks[i].op = mem;
        thread_stacks[i].tstack = (void *)stack;
        thread_stacks[i].inuse = 1;
        break;
      }
    }
  }

  return rc;
}

int
thread_join()
{
  void* stack;

  int rc = join(&stack);

  // Only free mem on success
  if (rc != -1)
  {
    for (int i=0; i<MAX_THREADS; i++)
    {
      // Looking for the corresponding entry in the map
      if(thread_stacks[i].tstack == stack && thread_stacks[i].inuse == 1)
      {
        // Freeing the original malloc'ed pointer
        free(thread_stacks[i].op);
        thread_stacks[i].op = 0;
        thread_stacks[i].tstack = 0;
        thread_stacks[i].inuse = 0;
        break;
      }
    }
  }
  
  return rc;
}