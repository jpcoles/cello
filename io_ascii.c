#include "env.h"
#include "io_ascii.h"
#include "mem.h"

//============================================================================
//                                 loadascii
//
// Load initial conditions from an ascii file in the format
//      nparticles
//      ptype pmass x y z vx vy vz
//      ptype pmass x y z vx vy vz
//         ...
//============================================================================
int load_ascii(char *filename)
{
    int i;

    ANNOUNCE_BEGIN(__FUNCTION__);

    FILE *fp = fopen(filename, "rt");
    if (fp == NULL) return 1;


    int di;
    double rx, ry, rz,
           vx, vy, vz,
           m;


    fscanf(fp, "%i\n", &di);
    env.n_particles = di;

    env.ps = CALLOC(particle_t, env.n_particles + 1);

    for (i=1; i <= env.n_particles; i++)
    {
        fscanf(fp, "%i %lf %lf %lf %lf %lf %lf %lf\n", &di, &m, &rx, &ry, &rz, &vx, &vy, &vz);

        id(i)   = i;
        soft(i) = DEFAULT_SOFTENING;

        rx(i) = rx;
        ry(i) = ry;
        rz(i) = rz;
        vx(i) = vx;
        vy(i) = vy;
        vz(i) = vz;

        //fprintf(stderr, "%i %f %f %f %f %f %f %f\n", di, M(i), rx(i), ry(i), rz(i), vx(i), vy(i), vz(i));
    }

    fclose(fp);

    ANNOUNCE_END(__FUNCTION__);
    return 0;
}

