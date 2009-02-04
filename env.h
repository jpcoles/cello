#ifndef ENV_H
#define ENV_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "macros.h"
#include "tree.h"
#include "gstypes.h"

#define AOS 1   /* Array of Structures  vs.  Structure of Arrays */

#if AOS

#define ID               uint32_t pid
#define POSITION         pos_t r
#define VELOCTY          vel_t v
#define ACCELERATION     acc_t a

typedef struct
{
    ID;
    POSITION;
    VELOCTY;
    ACCELERATION;
} particle_t;

#define PARTICLES        particle_t *ps

#define id(i) (env.ps[i].pid)

#define rx(i) (env.ps[i].r.x)
#define ry(i) (env.ps[i].r.y)
#define rz(i) (env.ps[i].r.z)

#define vx(i) (env.ps[i].v.x)
#define vy(i) (env.ps[i].v.y)
#define vz(i) (env.ps[i].v.z)

#define ax(i) (env.ps[i].a.x)
#define ay(i) (env.ps[i].a.y)
#define az(i) (env.ps[i].a.z)

//#define M(i) (1.0)
#define M(i) (1.0F / env.n_particles)

#define dt(i) (0.001)
//#define M(i) (env.ps[i].M)
//#define dt(i) (env.ps[i].dt)

#else /* AOS */

#define ID               uint32_t *pid
#define POSITION         pos_t *r
#define VELOCTY          vel_t *v
#define ACCELERATION     acc_t *a
#define PARTICLES        ID; POSITION; VELOCITY; ACCLERATION;

#define id(i) (env.pid[i])
#define rx(i) (env.r[i].x)
#define ry(i) (env.r[i].y)
#define rz(i) (env.r[i].z)
#define vx(i) (env.v[i].x)
#define vy(i) (env.v[i].y)
#define vz(i) (env.v[i].z)
#define ax(i) (env.a[i].x)
#define ay(i) (env.a[i].y)
#define az(i) (env.a[i].z)

#endif /* AOS */


typedef struct
{
    int rank, size;
} parallel_environment_t;

enum filetype_enum
{
    TIPSY_STANDARD,
    TIPSY_NATIVE,
};

typedef struct
{
    char *base_input_filename;
    enum filetype_enum input_filetype;

} config_t;


typedef struct
{
    parallel_environment_t pe;

    uint32_t current_step, total_steps;

    uint32_t n_particles;
    PARTICLES;

    tree_node_t *tree;
    uint32_t n_tree_nodes;
    uint32_t max_tree_nodes;

    float opening_angle;

    config_t cfg;

} env_t;

extern env_t env;

#endif

