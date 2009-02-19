#include <string.h>
#include "env.h"
#include "mem.h"
#include "io.h"

static char *fout_name = NULL;

static char *file_output_name()
{
    int len = strlen(env.cfg.base_output_filename) + 9;
    fout_name = REALLOC(fout_name, char, len);

    myassert(fout_name, "Unable to allocate memory for filename!");

    snprintf(fout_name, len, "%s.%05i", env.cfg.base_output_filename, env.current_step);

    return fout_name;
}

int store_timestep()
{
    switch (env.cfg.output_filetype)
    {
        case TIPSY_STANDARD:
            store_standard_tipsy(file_output_name());
            break;
        case TIPSY_NATIVE:
            store_native_tipsy(file_output_name());
            break;
        case ASCII:
            fprintf(stderr, "NO SUPPORT FOR WRITING ASCII FILES!\n");
            break;
    }

    return 0;
}

int load_timestep()
{
    switch (env.cfg.input_filetype)
    {
        case TIPSY_STANDARD:
            load_standard_tipsy(env.cfg.base_input_filename);
            break;
        case TIPSY_NATIVE:
            load_native_tipsy(env.cfg.base_input_filename);
            break;
        case ASCII:
            load_ascii(env.cfg.base_input_filename);
            break;
        default:
            myassert(0, "Tried to load file, but the filetype is wrong.");
            break;
    }

    return 0;
}
