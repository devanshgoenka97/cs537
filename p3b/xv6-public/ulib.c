#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"
#include "x86.h"

#define PAGESIZE    4096
#define MAX_THREADS 64

typedef struct __thread_stacks_t {
  void* op;
  void* tstack;
  int inuse;
} thread_stacks_t;

// Used for keeping track of thread stacks to be freed
thread_stacks_t thread_stacks[MAX_THREADS];

char*
strcpy(char *s, const char *t)
{
  char *os;

  os = s;
  while((*s++ = *t++) != 0)
    ;
  return os;
}

int
strcmp(const char *p, const char *q)
{
  while(*p && *p == *q)
    p++, q++;
  return (uchar)*p - (uchar)*q;
}

uint
strlen(const char *s)
{
  int n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}

void*
memset(void *dst, int c, uint n)
{
  stosb(dst, c, n);
  return dst;
}

char*
strchr(const char *s, char c)
{
  for(; *s; s++)
    if(*s == c)
      return (char*)s;
  return 0;
}

char*
gets(char *buf, int max)
{
  int i, cc;
  char c;

  for(i=0; i+1 < max; ){
    cc = read(0, &c, 1);
    if(cc < 1)
      break;
    buf[i++] = c;
    if(c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return buf;
}

int
stat(const char *n, struct stat *st)
{
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if(fd < 0)
    return -1;
  r = fstat(fd, st);
  close(fd);
  return r;
}

int
atoi(const char *s)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return n;
}

void*
memmove(void *vdst, const void *vsrc, int n)
{
  char *dst;
  const char *src;

  dst = vdst;
  src = vsrc;
  while(n-- > 0)
    *dst++ = *src++;
  return vdst;
}

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