#ifndef CELLO_TYPES_H
#define CELLO_TYPES_H

typedef float       real;
typedef double      real_accurate;
typedef long double real_very_accurate;

typedef real_accurate dist_t;

typedef struct { dist_t x, y, z; } xyz_t;
typedef xyz_t pos_t;
typedef xyz_t vel_t;
typedef xyz_t acc_t;
typedef real pot_t;
typedef real softening_t;
typedef real mass_t;
typedef real dt_t;

typedef struct
{
    dist_t min, max;
} range_t;

typedef struct
{
    range_t x, y, z;
} bound_t;

typedef uint64_t Pid_t;  /* Particle id type */
typedef uint32_t cid_t;  /* Tree cell id type */

#endif

