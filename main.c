#include <getopt.h>
#include <string.h>
#include <fenv.h>
#include "env.h"
#include "mem.h"
#include "nvidia.h"
#include "io.h"
#include "n2.h"
#include "fmm.h"
#include "sighandler.h"

env_t env;

int set_default_config()
{
    env.cfg.base_input_filename     = "";
    env.cfg.base_output_filename    = "";
    env.cfg.input_filetype          = NONE;
    env.cfg.output_filetype         = NONE;

    env.cfg.stopfile                = DEFAULT_STOPFILE;
    env.cfg.lockfile                = DEFAULT_STOPFILE;

    env.cfg.nrungs                  = DEFAULT_NRUNGS;

    env.cfg.overwrite               = 0;

    env.cfg.write_snapshot          = 1;
    env.cfg.write_rhoe              = 0;
    env.cfg.write_acc               = 0;

    env.cfg.base_timestep           = 0;

    env.cfg.total_steps             = 1;
    env.cfg.current_step            = 0;
    env.cfg.output_every            = 1;

    env.cfg.opening_angle           = DEFAULT_THETA;

    env.cfg.force_method            = FORCE_TREE;

    return 0;
}

int handle_command_line(int argc, char **argv)
{
    int c;

    while (1) 
    {
        int option_index = 0;
        static struct option long_options[] = {
           {"tipsy-std",        no_argument,       0, 0},
           {"mem-file",         no_argument,       0, 0},
           {"overwrite",        no_argument,       0, 0},
           {"no-overwrite",     no_argument,       0, 0},
           {"theta",            required_argument, 0, 0},
           {"steps",            required_argument, 0, 0},
           {"write-acc",        no_argument,       0, 0},
           {"every",            required_argument, 0, 0},
           {"force-n2",         no_argument,       0, 0},
           {0, 0, 0, 0}
        };

        c = getopt_long(argc, argv, "i:o:", long_options, &option_index);

        if (c == -1)
           break;

#define OPT(s) (!strcmp(s, long_options[option_index].name))
        switch (c) 
        {
            case 0:
                    if (OPT("tipsy-std"     ))   env.cfg.input_filetype = TIPSY_STANDARD;
               else if (OPT("tipsy-nat"     ))   env.cfg.input_filetype = TIPSY_NATIVE;
               else if (OPT("mem-file"      ))   env.cfg.input_filetype = MEM_FILE;
               else if (OPT("theta"         ))   env.cfg.opening_angle = atof(optarg);
               else if (OPT("steps"         ))   env.cfg.total_steps = atol(optarg);
               else if (OPT("overwrite"     ))   env.cfg.overwrite = 1;
               else if (OPT("no-overwrite"  ))   env.cfg.overwrite = 0;
               else if (OPT("write-acc"     ))   env.cfg.write_acc = 1;
               else if (OPT("write-snap"    ))   env.cfg.write_snapshot = 1;
               else if (OPT("no-write-snap" ))   env.cfg.write_snapshot = 0;
               else if (OPT("write-rhoe"    ))   env.cfg.write_rhoe = 1;
               else if (OPT("every"         ))   env.cfg.output_every = atol(optarg);
               else if (OPT("force-n2"      ))   env.cfg.force_method = FORCE_N2;

               break;

            case 'i':
                env.cfg.base_input_filename = MALLOC(char, strlen(optarg)+1);
                strcpy(env.cfg.base_input_filename, optarg); 
                break;
            case 'o':
                env.cfg.base_output_filename = MALLOC(char, strlen(optarg)+1);
                strcpy(env.cfg.base_output_filename, optarg); 
                break;
        }
    }

    return 0;
}

int fixup_config()
{
    if (env.cfg.input_filetype == NONE) 
    {
        env.cfg.input_filetype  = MEM_FILE;
        env.cfg.output_filetype = TIPSY_STANDARD;
        env.cfg.base_input_filename = "grid512";
    }

    if (env.cfg.output_filetype == NONE) env.cfg.output_filetype = env.cfg.input_filetype;

    if (env.cfg.total_steps == 0)
        env.cfg.write_snapshot = 0;

    return 0;
}

int main(int argc, char **argv)
{
    signal(SIGINT, sig_int);

    memset(&env, 0, sizeof(env));
    
    set_default_config();

    handle_command_line(argc, argv);

    fixup_config();


    env.current_step = env.cfg.current_step;
    env.base_timestep = env.cfg.base_timestep;

    //env.nrungs = env.cfg.nrungs;

    nvidia_init();

#if 0
    env.opening_angle       = .06;
    env.cfg.stopfile        = DEFAULT_STOPFILE;
    env.cfg.output_every    = 100;
    env.cfg.nrungs          = 0;

    env.total_steps         = 10000;
    env.current_step        = 0;
    env.base_timestep       = 1.00;
    env.nrungs              = 0;
#endif

    //env.cfg.base_output_filename = "Grid";
    //env.cfg.output_filetype = TIPSY_STANDARD;

    //------------------------------------------------------------------------

    log("ARCH", "sizeof(uint64_t)  = %ld\n", sizeof(uint64_t));
    log("ARCH", "sizeof(uint32_t)  = %ld\n", sizeof(uint32_t));
    log("ARCH", "sizeof(double)    = %ld\n", sizeof(double));
    log("ARCH", "sizeof(env.ps[0]) = %ld\n", sizeof(env.ps[0]));


    int (*int_startup)()       = fmm_startup;
    int (*step_particles)()    = fmm_step_particles;
    int (*calc_acceleration)() = fmm_calculate_acceleration;

    //------------------------------------------------------------------------

    //if (!stop_simulation())
    {
        if (load_timestep())
            ERROR("Can't load input.");

        log("SIM",  "Total Particles = %ld\n", (long int)env.n_particles);

        env.trees                 = CALLOC(tree_t, 2);
        env.trees[0].n_nodes      = 0;
        env.trees[0].l            = 1;
        env.trees[0].u            = env.n_particles;
        //env.trees[0].bucket_size  = 8;

        env.icrit.N_pre_cb  = 3;
        env.icrit.N_post_cb = 128;
        env.icrit.N_pre_cc  = 0;
        //env.icrit.N_cs      = 64;

        switch (env.cfg.force_method)
        {
            case FORCE_N2:
                env.icrit.N_cs            = env.n_particles;
                env.trees[0].bucket_size  = env.n_particles;
                env.icrit.N_post_cc       = 0;
                break;
            case FORCE_TREE:
                env.icrit.N_cs            = 64;
                env.trees[0].bucket_size  = 8;
                env.icrit.N_post_cc       = env.trees[0].bucket_size * env.trees[0].bucket_size;
                break;
        }


        //--------------------------------------------------------------------

        int_startup();

        if (env.cfg.total_steps == 0)
        {
            calc_acceleration();
            sort_particles();
            store_all();
        }
        else
        {
            while (!stop_simulation())
            {
                feclearexcept(FE_ALL_EXCEPT);

                env.current_step++;

                log("", BAR3"\n");
                log("STEP", "%i\n", env.current_step);

                step_particles();

        //      int raised = fetestexcept(FE_ALL_EXCEPT);
        //      myassert(!raised, "Floating-point exception occured in last step! flags:0x%x", raised);
         
                sort_particles();

                if ((env.current_step % env.cfg.output_every) == 0)
                    store_all();
            }

            if ((env.current_step % env.cfg.output_every) != 0)
                store_all();
        }
    }

    log("EXIT", "Success.\n");

    return 0;
}

