#ifndef MEM_H
#define MEM_H

#include <stdlib.h>


#define MALLOC(type, len) \
    (fprintf(stderr, "MALLOC:  %20s:%-4i  %10ziB for %5i " #type "\n", \
             __FUNCTION__,__LINE__,((len) * sizeof(type)),(len)),\
             ((type *)malloc((len) * sizeof(type))))
#define REALLOC(ptr, type, len) \
    (fprintf(stderr, "REALLOC: %20s:%-4i  %10ziB for %5i " #type "\n", \
             __FUNCTION__,__LINE__,((len) * sizeof(type)),(len)),\
             ((type *)realloc(ptr, (len) * sizeof(type))))
#define CALLOC(type, len) \
    (fprintf(stderr, "CALLOC:  %20s:%-4i  %10ziB for %5i " #type "\n", \
             __FUNCTION__,__LINE__,((len) * sizeof(type)),(len)),\
    ((type *)calloc(len, sizeof(type))))
#define FREE(ptr) \
    do { fprintf(stderr, "FREE:    %20s:%-4i  " #ptr "\n", __FUNCTION__,__LINE__); \
         free(ptr); } while(0)


#endif

