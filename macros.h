#ifndef MACROS_H
#define MACROS_H

#include <signal.h>
#include <math.h>

//----------------------------------------------------------------------------
// Kinds of debug messages
//----------------------------------------------------------------------------
#define DBG_NONE        0x0000
#define DBG_TREE        0x0001
#define DBG_INTERACT    0x0002
#define DBG_MEMORY      0x0004
#define DBG_EVAL        0x0008

#define DBG_KIND (DBG_NONE | DBG_EVAL)

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
    ((void)((_label[0]) && fprintf(stderr, "%15s | ", _label)), \
    fprintf(stderr, fmt, ##__VA_ARGS__))
#define dbgprintf(kind, fmt, ...) ((void)((DBG_IT(kind) && log("DEBUG", fmt, ##__VA_ARGS__))))

#define ERROR(...)        do { log("ERROR", ##__VA_ARGS__); exit(1);    } while (0)
#define ERROR1(code, ...) do { log("ERROR", ##__VA_ARGS__); exit(code); } while (0)

#define WARN(...) do { log("WARNING", ##__VA_ARGS__); } while (0)

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
        eprintf("Debug Assertion failed in %s() [ %s:%i ]\n" #test "\n", __FUNCTION__, __FILE__, __LINE__); \
        eprintf(__VA_ARGS__);                                                      \
        eprintf("\n" BAR2 "\n");                                                   \
        raise(SIGABRT); }
#else
#define dbgassert(test, ...)
#endif

#define myassert(test, ...) \
    if (!(test)) { \
        eprintf(BAR2 "\n"); \
        eprintf("Assertion failed in %s() [ %s:%i ]\n" #test "\n", __FUNCTION__, __FILE__, __LINE__); \
        eprintf(__VA_ARGS__); \
        eprintf("\n" BAR2 "\n"); \
        raise(SIGABRT); }


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define forall_particles(__i) \
    for ((__i)=1; (__i) <= env.n_particles; (__i)++)

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

#if 1
//#define ANNOUNCE_BEGIN(...) do { eprintf(BAR1 "\nBEGIN "); eprintf(__VA_ARGS__); eprintf("\n"); } while (0)
//#define ANNOUNCE_END(...)   do { eprintf("END  "); eprintf(__VA_ARGS__); eprintf("\n" BAR1 "\n"); } while (0)
#define ANNOUNCE_BEGIN(...) do { log("", "\n"); log("BEGIN FUNC", __VA_ARGS__); log("", "\n"); } while (0)
#define ANNOUNCE_END(...)   do { log("END FUNC", __VA_ARGS__); log("", "\n\n"); } while (0)
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

#ifdef FP_FAST_FMAF
#define FMA                     fmaf
#define DIST2(x,y,z)            FMA((x),(x), FMA((y),(y), POW((z),2)))
#define SOFTDIST2(x,y,z,eps)    FMA((x),(x), FMA((y),(y), FMA((z),(z), POW((eps),2))))
#else
#define FMA(x,y,z)              (((x)*(y))+(z))
#define DIST2(x,y,z)            (POW((x),2) + POW((y),2) + POW((z),2))
#define SOFTDIST2(x,y,z,eps)    (DIST2((x),(y),(z)) + POW((eps),2))
#endif

#define LDEXP                   ldexpf
#define SQRT                    sqrtf
#define POW                     powf

#define MIN(a,b)                (((a)<(b)) ? (a) : (b))


#define MAG(x,y,z)              SQRT(DIST2((x),(y),(z)))
#define DIST(x,y,z)             MAG((x),(y),(z))
#define SOFTDIST(x,y,z,eps)     SQRT(SOFTDIST2((x),(y),(z),(eps)))
    

#endif

