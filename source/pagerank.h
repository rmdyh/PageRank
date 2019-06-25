#ifndef _PAGERANK_
#define _PAGERANK_
#include<stdint.h>

#define BLOCK_SIZE 10000
#define EPSILON 1e-10
#define BETA 0.85
#define TOP 100

typedef struct{
    long pointer;
    uint16_t degree;
    uint16_t dest_num;
} LinkTable;

typedef int ToNode[1<<16];

int split_data(int,char **);
int solve();
int output();

#endif
