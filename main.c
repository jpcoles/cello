#include <string.h>
#include "env.h"
#include "mem.h"
#include "nvidia.h"
#include "io.h"
#include "n2.h"
#include "fmm.h"

env_t env;

int main(int argc, char **argv)
{
    uint32_t i=0;
    float x,y,z;

    memset(&env, 0, sizeof(env));

    nvidia_init();

    if (argc < 2)
    {
        env.ps = CALLOC(particle_t, 1000+1);

#define MAKE_GRID(start, end, inc) \
        for (z=start; z <= end; z += inc)           \
            for (y=start; y <= end; y += inc)       \
                for (x=start; x <= end; x += inc)   \
                {                                   \
                    i++;                            \
                    id(i) = i;                      \
                    rx(i) = x;                      \
                    ry(i) = y;                      \
                    rz(i) = z;                      \
                }

        MAKE_GRID(-0.4, 0.3, 0.1);
        //MAKE_GRID(-0.05, 0.05, 0.099999);

        env.n_particles = i;
    }
#if 0
    else
    {
        env.cfg.input_filetype = TIPSY_STANDARD;
        env.cfg.base_input_filename = MALLOC(char, strlen(argv[1])+1);
        strcpy(env.cfg.base_input_filename, argv[1]); 
        load_timestep();
    }
#else
    else
    {
        env.cfg.input_filetype = ASCII;
        env.cfg.base_input_filename = MALLOC(char, strlen(argv[1])+1);
        strcpy(env.cfg.base_input_filename, argv[1]); 
        load_timestep();
    }
#endif

    env.opening_angle = .6;
    env.cfg.stopfile = DEFAULT_STOPFILE;
    env.cfg.output_every = 100;
    env.total_steps  = 10000;
    env.current_step = 0;

    env.cfg.base_output_filename = "X";
    env.cfg.output_filetype = TIPSY_STANDARD;

    fprintf(stderr, "# Particles = %i\n", (int)env.n_particles);

    env.trees                 = MALLOC(tree_t, 2);
    env.trees[0].root         = NULL;
    env.trees[0].allocd_nodes = 0;
    env.trees[0].n_nodes      = 0;
    env.trees[0].l            = 1;
    env.trees[0].u            = env.n_particles;

#if 0

    build_oct_tree(&env.trees[0]);

    fill_tree(&env.trees[0]);


    //interact_prioq2();
    //interact_dehnen_modified();
    //interact_dehnen_modified2();
    //interact_dehnen_modified3();
    //interact_queue();
    //interact_prioq();

    interact_dehnen(&env.trees[0]);
    //print_oct_tree(&env.trees[0]);
    evaluate_dehnen(&env.trees[0]);
#endif

    //n2_update_acceleration();

    int (*int_startup)() = fmm_startup;
    int (*step_particles)() = fmm_step_particles;

    build_oct_tree(&env.trees[0]);
    fill_tree(&env.trees[0]);
    print_oct_tree(&env.trees[0]);

#if 0

    int_startup();
    while (!stop_simulation())
    {
        step_particles();

        env.current_step++;

        if ((env.current_step % env.cfg.output_every) == 0)
            store_timestep();

        //for (i=1; i < env.n_particles; i++)
            //printf("%.5f %.5f %.5f\n", ax(i), ay(i), az(i));
    }
#endif

#if 0
    printf("\n=======================================================================\n");

    for (i=1; i < env.n_particles; i++)
        printf("%.5f %.5f %.5f\n", ax(i), ay(i), az(i));
#endif

    return 0;
}

