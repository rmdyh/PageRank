#ifndef _PAGERANK_
#define _PAGERANK_
#include<stdint.h>

#define BLOCK_SIZE 10000        /* max size of r_new's block */
#define EPSILON 1e-10       /* condition of convergance */
#define BETA 0.85           /* teleports */
#define TOP 100             /* the number of webs have highest pagerank in result.txt */

int END = -1;               /* end mark in binary file */

int split_data(int,char **);/*break dataset into several blocks*/
int solve();                /*calculate the pagerank of each web*/
int output();               /*output the webs have highest pagerank into result.txt*/

#pragma warning(disable:4996)

#endif
