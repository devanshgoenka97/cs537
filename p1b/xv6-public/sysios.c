// Demonstrate that moving the "acquire" in iderw after the loop that
// appends to the idequeue results in a race.

// For this to work, you should also add a spin within iderw's
// idequeue traversal loop.  Adding the following demonstrated a panic
// after about 5 runs of stressfs in QEMU on a 2.1GHz CPU:
//    for (i = 0; i < 40000; i++)
//      asm volatile("");

#include "types.h"
#include "iostat.h"
#include "user.h"
#include "fcntl.h"

int
main(int argc, char *argv[])
{
  if(argc != 3){
    printf(1, "sysios: 2 command line arguments expected");
    exit();
  }

  int r = atoi(argv[1]);
  int w = atoi(argv[2]);

  int create_file = open("a.txt", O_CREATE | O_RDWR);

  char* garbagewrite = "h\n";
  
  for(int i=0; i<w; i++){
    write(create_file, garbagewrite, strlen(garbagewrite));
  }

  char buffer[255];

  for(int i=0; i<r; i++){
    read(create_file, buffer, strlen(garbagewrite));
  }

  struct iostat count;

  int getioc = getiocounts(&count);


  printf(1, "%d %d %d\n", getioc, count.readcount, count.writecount);

  close(create_file);

  exit();
}