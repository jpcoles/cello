#include <iostream>
#include <assert.h>
#include "ftipsy.hpp"
#include "io_tipsy.h"
#include "env.h"
#include "mem.h"

using namespace std;

int load_tipsy()
{
    ifTipsy in;

    TipsyHeader h;
    TipsyDarkParticle d;

    fprintf(stderr, "BEGIN Read tipsy file %s\n", env.cfg.base_input_filename);

    switch (env.cfg.input_filetype)
    {
        case TIPSY_STANDARD:
            in.open(env.cfg.base_input_filename, "standard");
            break;
        case TIPSY_NATIVE:
            in.open(env.cfg.base_input_filename, "native");
            break;
        default:
            myassert(0, "Tried to load a tipsy file, but the filetype is wrong.");
            break;
    }

    if (!in.is_open()) { assert(0); return 1; }

    in >> h;

    fprintf(stderr, "Dark: %16i\n", h.h_nDark);
    fprintf(stderr, "Star: %16i\n", h.h_nStar);
    fprintf(stderr, "Sph:  %16i\n", h.h_nSph);

    env.ps = CALLOC(particle_t, h.h_nDark + 1);

    in.seekg(tipsypos::header);
    in.seekg(tipsypos::dark);
    for (uint32_t i=1; i <= h.h_nDark; i++)
    {
        in >> d;

        id(i) = i;
        rx(i) = d.pos[0];
        ry(i) = d.pos[1];
        rz(i) = d.pos[2];

        vx(i) = d.vel[0];
        vy(i) = d.vel[1];
        vz(i) = d.vel[2];

        //M(i)  = d.mass;
    }

    in.close();

    env.n_particles = h.h_nDark;

    fprintf(stderr, "END  Read tipsy file\n");

    return 0;
}

