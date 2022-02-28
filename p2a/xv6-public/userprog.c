#include "types.h"
#include "stat.h"
#include "pstat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

int
main(int argc, char *argv[])
{
    struct pstat* stat = (struct pstat *) malloc(sizeof(struct pstat));

    getpinfo(stat);
    for(int i=0;i<64;i++){
        if(getpid() == stat->pid[i])
        {
            printf(1, "tickets : %d, boosted: %d\n", stat->tickets[i], stat->boostsleft[i]);
        }
    }

    int r;
    r = settickets(getpid(), 10);

    if (r<0)
        exit();

    getpinfo(stat);
    for(int i=0;i<64;i++){
        if(getpid() == stat->pid[i])
        {
            printf(1, "tickets : %d, boosted: %d\n", stat->tickets[i], stat->boostsleft[i]);
        }
    }

    sleep(1000);
    int i = 0, j = 0;
    while (i < 800000) {
        j += i * j + 1;
        i++;
    }

    getpinfo(stat);
    for(int i=0;i<64;i++){
        if(getpid() == stat->pid[i])
        {
            printf(1, "tickets : %d, boosted: %d\n", stat->tickets[i], stat->boostsleft[i]);
        }
    }

    exit();
}
