#include <math.h>
#include "env.h"

static float dt=0;

static inline int DRIFT()
{
    Pid_t i;
    for (i=1; i <= env.n_particles; i++)
    {
        rx(i) += vx(i) * dt;
        ry(i) += vy(i) * dt;
        rz(i) += vz(i) * dt;
    }
    return 0;
}

static inline int KICK()
{
    Pid_t i;
    for (i=1; i <= env.n_particles; i++)
    {
        vx(i) += 0.5 * ax(i) * dt;
        vy(i) += 0.5 * ay(i) * dt;
        vz(i) += 0.5 * az(i) * dt;
    }
    return 0;
}

static int n2_update_acceleration()
{
    Pid_t i,j;

    for (i=1; i <= env.n_particles; i++)
    {
        ax(i) = 0;
        ay(i) = 0;
        az(i) = 0;
    }

    for (i=1; i < env.n_particles; i++)
    {
        for (j=i+1; j <= env.n_particles; j++)
        {
            const dist_t dx = rx(j)-rx(i);
            const dist_t dy = ry(j)-ry(i);
            const dist_t dz = rz(j)-rz(i);
            const dist_t Rinv = 1.0 / DIST(dx,dy,dz);

            ax(i) += (((M(j) * dx) * Rinv) * Rinv) * Rinv;
            ay(i) += (((M(j) * dy) * Rinv) * Rinv) * Rinv;
            az(i) += (((M(j) * dz) * Rinv) * Rinv) * Rinv;

            ax(j) -= (((M(i) * dx) * Rinv) * Rinv) * Rinv;
            ay(j) -= (((M(i) * dy) * Rinv) * Rinv) * Rinv;
            az(j) -= (((M(i) * dz) * Rinv) * Rinv) * Rinv;
        }
    }

    return 0;
}

int n2_startup()
{
    n2_update_acceleration();
    return 0;
}

int n2_step_particles()
{
    dt = 0.001;
    KICK();
    DRIFT();
    n2_update_acceleration();
    KICK();

    return 0;
}
