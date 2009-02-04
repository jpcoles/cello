#include <stdio.h>
#include <cuda.h>
//#include <cutil.h>
#include "nvidia.h"


int nvidia_init()
{
    int i;
    int deviceCount;
    struct cudaDeviceProp prop;
    cudaError_t err;

    cudaGetDeviceCount(&deviceCount);

    fprintf(stderr, "------------------------------------------------------------------------------\n");
    fprintf(stderr, "  NVIDIA GPU Driver -- Found %i device(s).\n", deviceCount);

    for (i=0; i < deviceCount; i++)
    {
        fprintf(stderr, "\n\tDevice %i\n", i);

        err = cudaGetDeviceProperties(&prop, i);
        if (err != cudaSuccess)
        {
            fprintf(stderr, "\tError %i\n", err);
            continue;
        }

        fprintf(stderr, 
               "\tName: '%s'\n"
               "\tTotal Global Memory: %u\n"
               "\tShared Memory Per Block: %u\n"
               "\tRegisters Per Block: %i\n"
               "\tWarp Size: %i\n"
               "\tMemory Pitch: %u\n"
               "\tMaximum Threads Per Block: %i\n"
               "\tMaximum Size of Each Block Dimension: %5i %5i %5i\n"
               "\tMaximum Size of Each Grid Dimension:  %5i %5i %5i\n"
               "\tTotal Constant Memory: %i\n"
               "\tRevision: %i.%i\n"
               "\tClockrate: %iHz\n"
               "\tSize of Property Structure: %u\n", 
               prop.name,
               prop.totalGlobalMem,
               prop.sharedMemPerBlock,
               prop.regsPerBlock,
               prop.warpSize,
               prop.memPitch,
               prop.maxThreadsPerBlock,
               prop.maxThreadsDim[0],
               prop.maxThreadsDim[1],
               prop.maxThreadsDim[2],
               prop.maxGridSize[0],
               prop.maxGridSize[1],
               prop.maxGridSize[2],
               prop.totalConstMem,
               prop.major,
               prop.minor,
               prop.clockRate,
               sizeof(prop));

        cudaSetDevice(i);
    }

    fprintf(stderr, "------------------------------------------------------------------------------\n");

    return 0;
}

