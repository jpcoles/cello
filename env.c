#include "macros.h"
#include "env.h"
#include "mem.h"


int stop_simulation()
{
    FILE *fp;
    int stop = 0;

    stop = stop || env.current_step == env.total_steps;
    stop = stop || ((fp=fopen(env.cfg.stopfile, "rb")) && fclose(fp));

    return stop;	
}

int sort_particles()
{
    int _psort_cmp(const void *a0, const void *b0)
    {
        const particle_t *a = (particle_t *)a0;
        const particle_t *b = (particle_t *)b0;
        if (a->pid > b->pid) return +1;
        if (a->pid < b->pid) return -1;
        myassert(0, "Particles %i and %i have the same sid!", (int)a->pid, (int)b->pid);
        return 0;
    }

    qsort(env.ps+1, env.n_particles, sizeof(env.ps[0]), _psort_cmp);
    return 0;
}

