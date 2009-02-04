#include <string.h>
#include "env.h"
#include "mem.h"
#include "io_tipsy.h"
#include "nvidia.h"

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
    else
    {
        env.cfg.input_filetype = TIPSY_STANDARD;
        env.cfg.base_input_filename = MALLOC(char, strlen(argv[1])+1);
        strcpy(env.cfg.base_input_filename, argv[1]); 
        load_tipsy();
    }

    env.opening_angle = .6;

    fprintf(stderr, "# Particles = %i\n", env.n_particles);

    build_oct_tree();

    fill_tree();

    //print_oct_tree();

    //interact_prioq2();
    interact_dehnen_modified();
    //interact_dehnen_modified2();
    //interact_dehnen_modified3();
    //interact_dehnen();
    //interact_queue();
    //interact_prioq();

    return 0;
}

