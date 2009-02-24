#ifndef MACROS_H
#define MACROS_H

#include <math.h>

//----------------------------------------------------------------------------
// Kinds of debug messages
//----------------------------------------------------------------------------
#define DBG_NONE        0x0000
#define DBG_TREE        0x0001
#define DBG_INTERACT    0x0002
#define DBG_MEMORY      0x0004

#define DBG_KIND DBG_NONE

#define DBG_IT(_kind) ((DBG_KIND & (_kind)) == (_kind))
#define DBG(_kind) if (DBG_IT(_kind))

//----------------------------------------------------------------------------
// Printing an logging macros.
//----------------------------------------------------------------------------
#define BAR1 "------------------------------------------------------------------------------"
#define BAR2 "=============================================================================="
#define BAR3 "::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::"
#define BAR4 "******************************************************************************"
#define BAR5 "______________________________________________________________________________"

#define eprintf(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#define log(_label, fmt, ...) \
    ((_label[0]) && fprintf(stdout, "%-15s", _label), \
    fprintf(stdout, fmt, ##__VA_ARGS__))
#define dbgprintf(kind, fmt, ...) (DBG_IT(kind) && log("DBG", fmt, ##__VA_ARGS__))

//----------------------------------------------------------------------------
// There are some asserts that are really only necessary for debugging.  This
// turns them on or off. This of course may affect inlining and performance in
// general.
//----------------------------------------------------------------------------
#define DBG_ASSERTS_ON 0

#if DBG_ASSERTS_ON
#define dbgassert(test, ...) \
    if (!(test)) {                                                                 \
        eprintf(BAR2 "\n");                                                        \
        eprintf("Debug Assertion failed. %s:%i\n" #test "\n", __FILE__, __LINE__); \
        eprintf(__VA_ARGS__);                                                      \
        eprintf("\n" BAR2 "\n");                                                   \
        exit(1); }
#else
#define dbgassert(test, ...)
#endif

#define myassert(test, ...) \
    if (!(test)) { \
        eprintf(BAR2 "\n"); \
        eprintf("Assertion failed. %s:%i\n" #test "\n", __FILE__, __LINE__); \
        eprintf(__VA_ARGS__); \
        eprintf("\n" BAR2 "\n"); \
        exit(1); }


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define for_all_block_particles(__i) \
    for (__i=0; __i < env.blk.size; __i++)

#define for_all_block_particles_except(__i, __except) \
    for (__i=(0==__except); __i < env.blk.size; __i += 1 + ((__i+1)==(__except)))

#define for_all_nodes(__i) \
    for (__i=0; __i < env.pe.size; __i++)

#define for_all_nodes_except(__i,__except) \
    for (__i=(0==__except); __i < env.pe.size; __i += 1 + ((__i+1)==(__except)))


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//#define ANNOUNCE_BEGIN(name) do { fprintf(stderr, "BEGIN %s" BAR1 "\n",    name); } while (0)
//#define ANNOUNCE_END(name)   do { fprintf(stderr, "END   %s" BAR1 "\n", name); } while (0)

#if 0
#define ANNOUNCE_BEGIN(...) do { eprintf(BAR1 "\nBEGIN "); eprintf(__VA_ARGS__); eprintf("\n"); } while (0)
#define ANNOUNCE_END(...)   do { eprintf("END  "); eprintf(__VA_ARGS__); eprintf("\n" BAR1 "\n"); } while (0)
#else
#define ANNOUNCE_BEGIN(...) 
#define ANNOUNCE_END(...)
#endif

//----------------------------------------------------------------------------
// Allocation/Deallocation of memory.
//----------------------------------------------------------------------------

#define MALLOC(type, len) \
    ((void)dbgprintf(DBG_MEMORY, "MALLOC:  %20s:%-4i  %10zi for %5i " #type "\n", \
             __FUNCTION__,__LINE__,((size_t)(len) * sizeof(type)),(uint32_t)(len)),\
    ((type *)malloc((len) * sizeof(type))))

#define REALLOC(ptr, type, len) \
    ((void)dbgprintf(DBG_MEMORY, "REALLOC: %20s:%-4i  %10zi for %5i %-15s -- " #ptr "\n", \
             __FUNCTION__,__LINE__,((size_t)(len) * sizeof(type)),(uint32_t)(len), "" #type),\
    ((type *)realloc((ptr), (len) * sizeof(type))))

#define CALLOC(type, len) \
    ((void)dbgprintf(DBG_MEMORY, "CALLOC:  %20s:%-4i  %10zi for %5i " #type "\n", \
             __FUNCTION__,__LINE__,((size_t)(len) * sizeof(type)),(uint32_t)(len)),\
    ((type *)calloc(len, sizeof(type))))

#define FREE(ptr) \
    do { (void)dbgprintf(DBG_MEMORY, "FREE:    %20s:%-4i  -- " #ptr "\n", __FUNCTION__,__LINE__); \
         free(ptr); } while(0)

//----------------------------------------------------------------------------
// Useful physics macros.
//----------------------------------------------------------------------------

#define MIN(a,b) (((a)<(b)) ? (a) : (b))

#define DIST(x,y,z) sqrt(pow((x),2) + pow((y),2) + pow((z),2))
#define DIST2(x,y,z) (pow((x),2) + pow((y),2) + pow((z),2))

    
#endif

