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

