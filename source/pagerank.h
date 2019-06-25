#ifndef _PAGERANK_
#define _PAGERANK_
#include<stdint.h>

#define BLOCK_SIZE 10000    /*max size of r_new's block*/
#define EPSILON 1e-10       /*condition of convergance*/
#define BETA 0.85           /*teleports*/
#define TOP 100             /*the number of webs have highest pagerank in result.txt*/

typedef struct{
    long pointer;           /*point to the location of r_old*/
    uint16_t degree;        /*degree*/
    uint16_t dest_num;      /*the number of records in this block*/
} LinkTable;

typedef int ToNode[1<<16];  /*records in one LinkTable*/

int split_data(int,char **);/*break dataset into several blocks*/
int solve();                /*calculate the pagerank of each web*/
int output();               /*output the webs have highest pagerank into result.txt*/

#endif
