#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "pstat.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_settickets(void)
{
  int pid;
  int tickets;

  if(argint(0, &pid) < 0 || argint(1, &tickets) < 0 || tickets <= 0)
    return -1;

  return settickets(pid, tickets);
}

int
sys_srand(void)
{
  int seed;

  if(argint(0, &seed) < 0)
    return -1;

  cprintf("rseed = %d\n", seed);

  setseed(seed);

  return 0;
}

int
sys_getpinfo(void)
{
  struct pstat* stat;
  if(argptr(0, (void *)&stat, sizeof(struct pstat *)) < 0)
    return -1;

  return getpinfo(stat);
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  
  // Setting the number of ticks to be slept
  myproc()->ticks_to_sleep = n;
  myproc()->ticks_slept = 0;

  acquire(&tickslock);
  ticks0 = ticks;

  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);

  // cleanup
  myproc()->ticks_to_sleep = 0;
  myproc()->ticks_slept = 0;

  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
