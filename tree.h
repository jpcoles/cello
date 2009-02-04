#ifndef TREE_H
#define TREE_H

#include "gstypes.h"

typedef struct
{
    float min, max;
} range_t;

typedef struct
{
    range_t x, y, z;
} bound_t;

typedef struct 
{
    uint32_t id;
    int32_t l,u;            /* lower and upper particle id's in this node */
    uint32_t size;          /* Number of particles in the node */
    bound_t bnd;            /* Bounds of the node */
    pos_t r;                /* Center of the node */
    uint32_t children[8];   /* Indices of child nodes */
    uint32_t parent;

    float M;
    float rmax;
    pos_t cm;               /* Center of mass */

    //MOMR momr; 
    //LOCR locr;

} tree_node_t;

typedef struct
{
    uint32_t pid;
    pos_t r;
} particulate_t;

/* After a partition i always points to the first element on
   the right side of the partition. i-1 is therefore the last
   element on the left side. j should be ignored as it may
   be equal to i or i-1.
*/
#define PARTITION_SWAP(i,j,A,B,T) { T = A; A = B; B = T; }
#define PARTITION(P,T,i,j, LOWER_CMP, UPPER_CMP) \
do {\
    while ((i) <= (j) && (LOWER_CMP)) { ++(i); } \
    while ((i) <= (j) && (UPPER_CMP)) { --(j); } \
    while ((i) < (j)) { \
        PARTITION_SWAP(i,j, P[(i)], P[(j)], T); \
        while (++(i), (LOWER_CMP)) { } \
        while (--(j), (UPPER_CMP)) { } \
    }\
} while(0)

/* Loops over all node children putting the child index in _i and
   the value in _val.
*/
#define forall_tree_node_children(_N, _i, _val) \
    for (_i=0; (_val=node[_N].children[_i]) != 0 && _i < 8; _i++)

/* Loops over all node children pairs (including identical pairs) 
   putting the pair indices in _iA,_iB, and the values in _valA,_valB.
*/
#define forall_tree_node_child_pairs(_N, _iA, _iB, _valA, _valB) \
    for (_iA=0; (_valA=node[_N].children[_iA]) != 0 && _iA < 8; _iA++) \
        for (_iB=_iA; (_valB=node[_N].children[_iB]) != 0 && _iB < 8; _iB++)

int can_interact(uint32_t A, uint32_t B);
int print_oct_tree();
int build_oct_tree();
int interact_dehnen();
int interact_prioq();
int interact_queue();
int fill_tree();
int interact_dehnen_modified();
int interact_prioq2();
int interact_dehnen_modified2();
int interact_dehnen_modified3();

#endif

