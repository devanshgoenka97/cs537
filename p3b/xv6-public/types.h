typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef uint pde_t;

typedef struct __lock_t {
    int ticket;
    int current_turn;
} lock_t;
