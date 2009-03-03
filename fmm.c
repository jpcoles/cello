#include <fenv.h>
#include "env.h"
#include "fmm.h"
#include "tree.h"
#include "rungs.h"

static inline int DRIFT()
{
    Pid_t i;
    forall_particles(i)
    {
        rx(i) = FMA(vx(i), dt(i), rx(i));
        ry(i) = FMA(vy(i), dt(i), ry(i));
        rz(i) = FMA(vz(i), dt(i), rz(i));
    }
    return 0;
}

static inline int KICK()
{
    Pid_t i;
    forall_particles(i)
    {
        vx(i) = FMA(0.5 * ax(i), dt(i), vx(i));
        vy(i) = FMA(0.5 * ay(i), dt(i), vy(i));
        vz(i) = FMA(0.5 * az(i), dt(i), vz(i));
    }
    return 0;
}

int fmm_startup()
{
    Pid_t i;
    forall_particles(i)
    { 
        rung(i) = 1;
        ax(i) = ay(i) = az(i) = pot(i) = 0;
    }
    return 0;
}

int fmm_calculate_acceleration()
{
    Pid_t i;
    forall_particles(i)
    {
        ax(i) = ay(i) = az(i) = pot(i) = 0;
        rhoe(i) = 0;
    }

    build_oct_tree(&env.trees[0]);
    fill_tree(&env.trees[0]);
    interact_dehnen(&env.trees[0]);
    evaluate_dehnen(&env.trees[0]);

    return 0;
}

int fmm_step_particles()
{
    KICK();

    DRIFT();

    fmm_calculate_acceleration();
    adjust_rungs();

    KICK();

    return 0;
}


