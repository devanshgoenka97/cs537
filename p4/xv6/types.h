#ifndef TYPES_H
#define TYPES_H
typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef uint pde_t;
typedef struct __ws_node_t {
    int used;
    uint pte;
} wsnode_t;
#endif
