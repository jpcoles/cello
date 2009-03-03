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
    log("IO", "Loading %s\n", fname);
    log("TIPSY", "Standard format.\n");
    ifTipsy in;
    in.open(fname, "standard");
    load_tipsy(in);
    in.close();
    log("IO", "File loaded.\n");
    return 0;
}

int load_native_tipsy(char *fname)
{
    log("IO", "Loading %s\n", fname);
    log("TIPSY", "Native format.\n");
    ifTipsy in;
    in.open(fname, "native");
    load_tipsy(in);
    in.close();
    log("IO", "File loaded.\n");
    return 0;
}

static int load_tipsy(ifTipsy &in)
{
    TipsyHeader h;
    TipsyDarkParticle d;
    TipsyGasParticle  g;
    TipsyStarParticle s;

    if (!in.is_open()) 
    { 
        ERROR("Can't open tipsy file.");
        return 0;
    }

    in >> h;

    log(" ", "Dark: %16i\n", h.h_nDark);
    log(" ", "Star: %16i\n", h.h_nStar);
    log(" ", "Sph:  %16i\n", h.h_nSph);

    env.ps = CALLOC(particle_t, h.h_nBodies + 1);

    in.seekg(tipsypos::header);
#define READ(j, num, var) \
    for (uint32_t i=0; i < num; i++, j++) { \
        in >> var;        \
        id(j) = j;        \
        rx(j) = var.pos[0]; \
        ry(j) = var.pos[1]; \
        rz(j) = var.pos[2]; \
        vx(j) = var.vel[0]; \
        vy(j) = var.vel[1]; \
        vz(j) = var.vel[2]; \
         M(j) = var.mass;   \
        soft(j) = var.eps; eprintf("%f\n", soft(j));}

    Pid_t j=1;
    //READ(j, h.h_nSph, g);
    if (h.h_nDark) in.seekg(tipsypos::dark);
    READ(j, h.h_nDark, d);
    if (h.h_nStar) in.seekg(tipsypos::star);
    READ(j, h.h_nStar, s);

    env.n_particles = j-1;

    return 0;
}

#if 1
int store_standard_tipsy(char *fname)
{
    log("IO", "Storing %s\n", fname);
    ofTipsy out;
    out.open(fname, "standard");
    store_tipsy(out);
    out.close();
    return 0;
}

int store_native_tipsy(char *fname)
{
    log("IO", "Storing %s\n", fname);
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
        d.eps    = soft(i);

        out << d;
    }

    //fprintf(stderr, "END  Read tipsy file\n");
    ANNOUNCE_END(__FUNCTION__);

    return 0;
}
#endif

