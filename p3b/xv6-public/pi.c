/* test pi estimation; produce-consume pattern to intentionally introduce contention */

#include "types.h"
#include "user.h"
#include "x86.h"

#undef NULL
#define NULL ((void*)0)

#define PGSIZE (4096)
#define BUFFSIZE 5
#define NUM_THREADS 8
#define RAND_MAX 0x7fffffff
uint rseed = 0;

// https://rosettacode.org/wiki/Linear_congruential_generator
uint rand() {
    return rseed = (rseed * 1103515245 + 12345) & RAND_MAX;
}

int TRAILS = 500;

int trails_buf[BUFFSIZE];
int success;
int trail_i;

int ppid;
lock_t lock;

uint running_flag;

#define assert(x) if (x) {} else { \
   printf(1, "%s: %d ", __FILE__, __LINE__); \
   printf(1, "assert failed (%s)\n", # x); \
   printf(1, "TEST FAILED\n"); \
   kill(ppid); \
   exit(); \
}

void produce(void *arg1, void *arg2);
void consume(void *arg1, void *arg2);

int
main(int argc, char *argv[])
{
  int i;
  ppid = getpid();
  lock_init(&lock);

  trail_i = 0;
  success = 0;
  running_flag = 1;
  
  for(i = 0; i < NUM_THREADS; i++) {
    assert((thread_create(produce, &TRAILS, NULL)) > 0);
    assert((thread_create(consume, NULL, NULL)) > 0);
  }
  
  
  sleep(2000);
  running_flag = 0;
  sleep(100);
  
  
  for(i = 0; i < NUM_THREADS * 2; i++) {
    int join_pid = thread_join();
	assert(join_pid > 0);
  }
  
  //printf(1, "%d trails inside the circle out of %d trails\n", success, TRAILS * NUM_THREADS);
  int low = 3.13 * (TRAILS * NUM_THREADS) / 4;
  int high = 3.15 * TRAILS * NUM_THREADS / 4;
  
  //should be fairly close
  if(success < high && success > low)
    printf(1, "TEST PASSED\n");
  exit();
}

/*
 * to estimate pi, suppose we have a circle centered at (0, 0) with r = 1
 * let it be externally tangenet to a square centered at  (0, 0) with a = 2
 * 
 * A sandom points inside the square has 3.14/4 chance for the
 * point to be inside the circle. Given enough samples, 
 * the ratio should be fairly close. 
 */
void
produce(void *arg1, void *arg2) {
  int trails = *(int*)arg1;
  while(running_flag) {
    if (trails == 0)
      break;
    else {
      trails--;
	  
      int x = rand() % 100 * 2 - 100;
      int y = rand() % 100 * 2 - 100;
      int res = (x * x + y * y <= 10000) ? 1 : 0;
      //wait until the buffer has space	  
      while(1) {
        lock_acquire(&lock);
	if (trail_i < BUFFSIZE) {
	  trails_buf[trail_i] = res;
	  trail_i++;
    lock_release(&lock);
	  break;
	}
  lock_release(&lock);
      }
    }
  
  }
  
  exit();
}

void
consume(void *arg1, void *arg2) {
  while(running_flag) {
    int res = 0;
    
    lock_acquire(&lock);
    if (trail_i > 0) {
	  trail_i--;
	  res = trails_buf[trail_i];
    }
    success+=res;
    lock_release(&lock);
    
  }
  
  exit();
}
