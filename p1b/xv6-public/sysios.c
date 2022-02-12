/**
 * Copyright 2022, <Devansh Goenka>
 * Utility to demonstrate the "getiocounts" system call.
 * 
 * Usage: sysios m n
 * 
 * m: the number of reads to be performed
 * n: the number of writes to be performed
 * 
 * Returns:
 * a new line containing three integers, the return code for getiocounts
 * along with the number of reads and writes as returned by the system call
 * 
 * 
 **/

#include "types.h"
#include "iostat.h"
#include "user.h"
#include "fcntl.h"

int
main(int argc, char *argv[]){

  // The utility expects just two arguments, m and n.
  if(argc != 3){
    printf(1, "sysios: 2 command line arguments expected");
    exit();
  }

  // Passing the command line arguments
  int r = atoi(argv[1]);
  int w = atoi(argv[2]);

  // Creating a dummy file to read and write from
  int create_file = open("a.txt", O_CREATE | O_RDWR);

  char* garbagewrite = "h\n";
  
  // Writing to the file for 'n' times
  for(int i=0; i<w; i++){
    write(create_file, garbagewrite, strlen(garbagewrite));
  }

  char buffer[255];

  // Reading from the file 'm' times
  for(int i=0; i<r; i++){
    read(create_file, buffer, strlen(garbagewrite));
  }

  struct iostat count;

  // Invoking the getiocounts system call to fetch
  // the number of reads and writes by the current process
  int getioc = getiocounts(&count);

  // Printing the output received
  printf(1, "%d %d %d\n", getioc, count.readcount, count.writecount);

  // Closing the opened file descriptor
  close(create_file);

  exit();
}