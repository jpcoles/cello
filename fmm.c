#include "env.h"
#include "fmm.h"

static inline int DRIFT()
{
    Pid_t i;
    for (i=1; i <= env.n_particles; i++)
    {
        rx(i) += vx(i) * dt(i);
        ry(i) += vy(i) * dt(i);
        rz(i) += vz(i) * dt(i);
    }
    return 0;
}

static inline int KICK()
{
    Pid_t i;
    for (i=1; i <= env.n_particles; i++)
    {
        vx(i) += 0.5 * ax(i) * dt(i);
        vy(i) += 0.5 * ay(i) * dt(i);
        vz(i) += 0.5 * az(i) * dt(i);
    }
    return 0;
}

int fmm_startup()
{
    return 0;
}

int fmm_step_particles()
{
    Pid_t i;
    KICK();
    DRIFT();
        for (i=1; i <= env.n_particles; i++)
            ax(i) = ay(i) = az(i) = 0;

        build_oct_tree(&env.trees[0]);
        fill_tree(&env.trees[0]);
        print_oct_tree(&env.trees[0]);
        interact_dehnen(&env.trees[0]);
        evaluate_dehnen(&env.trees[0]);

        adjust_rungs();

    KICK();

    return 0;
}


