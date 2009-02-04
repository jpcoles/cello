#ifndef MACROS_H
#define MACROS_H

#include <math.h>

#define DBG_LVL 1

#define DBG(_level) if (DBG_LVL >= _level)

#define DBG_TREE 100
#define DBG_INTERACT 101

#define myassert(test, msg) \
    if (!(test)) { \
        fprintf(stderr, "==============================================================================\n"); \
        fprintf(stderr, "Assertion failed. %s:%i\n%s\n", \
                        __FILE__, __LINE__, msg); \
        fprintf(stderr, "==============================================================================\n"); \
        exit(1); }

#define MIN(a,b) (((a)<(b)) ? (a) : (b))

#define for_all_block_particles(__i) \
    for (__i=0; __i < env.blk.size; __i++)

#define for_all_block_particles_except(__i, __except) \
    for (__i=(0==__except); __i < env.blk.size; __i += 1 + ((__i+1)==(__except)))

#define for_all_nodes(__i) \
    for (__i=0; __i < env.pe.size; __i++)

#define for_all_nodes_except(__i,__except) \
    for (__i=(0==__except); __i < env.pe.size; __i += 1 + ((__i+1)==(__except)))


#define DIST(x,y,z) sqrt(pow((x),2) + pow((y),2) + pow((z),2))
#define DIST2(x,y,z) (pow((x),2) + pow((y),2) + pow((z),2))
    
#endif

