#include "types.h"
#include "x86.h"
#include "user.h"

void
lock_init(lock_t* lk)
{
  lk->current_turn = 0;
  lk->ticket = 0;
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
// Holding a lock for a long time may cause
// other CPUs to waste time spinning to acquire it.
void
lock_acquire(lock_t* lk)
{
  // Get ticket
  int myturn = fetch_and_add(&lk->ticket, 1);

  // Atomic checking in the while loop
  while (fetch_and_add(&lk->current_turn, 0) != myturn)
    ;

  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that the critical section's memory
  // references happen after the lock is acquired.
  __sync_synchronize();
}

// Release the lock.
void
lock_release(lock_t* lk)
{
  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that all the stores in the critical
  // section are visible to other cores before the lock is released.
  // Both the C compiler and the hardware may re-order loads and
  // stores; __sync_synchronize() tells them both not to.
  __sync_synchronize();

  // Atomically increase the turn and release the lock
  fetch_and_add(&lk->current_turn, 1);
}
