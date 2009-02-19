#include <iostream>
#include <assert.h>
#include "ftipsy.hpp"
#include "io_tipsy.h"
#include "env.h"
#include "mem.h"

using namespace std;

static int load_tipsy(ifTipsy &in);
static int store_tipsy(ofTipsy &out);

int load_standard_tipsy(char *fname)
{
    ifTipsy in;
    in.open(fname, "standard");
    load_tipsy(in);
    in.close();
    return 0;
}

int load_native_tipsy(char *fname)
{
    ifTipsy in;
    in.open(fname, "native");
    load_tipsy(in);
    in.close();
    return 0;
}

static int load_tipsy(ifTipsy &in)
{
    TipsyHeader h;
    TipsyDarkParticle d;

    if (!in.is_open()) { assert(0); return 1; }

    fprintf(stderr, "BEGIN Read tipsy file %s\n", env.cfg.base_input_filename);

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

    env.n_particles = h.h_nDark;

    fprintf(stderr, "END  Read tipsy file\n");

    return 0;
}

#if 1
int store_standard_tipsy(char *fname)
{
    ofTipsy out;
    out.open(fname, "standard");
    store_tipsy(out);
    out.close();
    return 0;
}

int store_native_tipsy(char *fname)
{
    ofTipsy out;
    out.open(fname, "native");
    store_tipsy(out);
    out.close();
    return 0;
}

static int store_tipsy(ofTipsy &out)
{
    TipsyHeader h;
    TipsyDarkParticle d;

    if (!out.is_open()) { assert(0); return 1; }

    //fprintf(stderr, "BEGIN Read tipsy file %s\n", env.cfg.base_input_filename);
    ANNOUNCE_BEGIN(__FUNCTION__);


    h.h_nDark = env.n_particles;
    h.h_nStar = 0;
    h.h_nSph  = 0;
    h.h_nBodies = h.h_nDark + h.h_nStar + h.h_nSph;

    h.h_nDims = 3;
    h.h_time  = 0;

    out << h;

    //in.seekg(tipsypos::header);
    out.seekp(tipsypos::dark);
    for (uint32_t i=1; i <= h.h_nDark; i++)
    {
        d.pos[0] = rx(i);
        d.pos[1] = ry(i);
        d.pos[2] = rz(i);

        d.vel[0] = vx(i);
        d.vel[1] = vy(i);
        d.vel[2] = vz(i);

        d.mass   = M(i);

        out << d;
    }

    //fprintf(stderr, "END  Read tipsy file\n");
    ANNOUNCE_END(__FUNCTION__);

    return 0;
}
#endif

