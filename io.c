#include <stdio.h>
#include <string.h>
#include "env.h"
#include "mem.h"
#include "io.h"
#include "memfiles.h"

static char *fout_name = NULL;

static char *tagged_output_name(char *tag)
{
    myassert(tag != NULL, "No null tags allowed. Use \"\" instead.");

    char *base = env.cfg.base_output_filename;

    if (base == NULL || base[0] == '\0')
        base = env.cfg.base_input_filename;

    int len = strlen(base) + strlen(tag) + 9;
    fout_name = REALLOC(fout_name, char, len);

    myassert(fout_name, "Unable to allocate memory for filename!");

    if (tag[0]) snprintf(fout_name, len, "%s.%s.%05i", base, tag, env.current_step);
    else        snprintf(fout_name, len,    "%s.%05i", base,      env.current_step);

    return fout_name;
}

static char *file_output_name()
{
    return tagged_output_name("");
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
        case MEM_FILE:
            load_memfile(env.cfg.base_input_filename);
            break;
        default:
            myassert(0, "Tried to load file, but the filetype is wrong.");
            break;
    }

    return 0;
}

int store_all()
{
    if (env.cfg.write_snapshot) store_timestep();
    if (env.cfg.write_acc)      store_a();
    if (env.cfg.write_rhoe)     store_rhoe();
    return 0;
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
            ERROR("NO SUPPORT FOR WRITING ASCII FILES!\n");
            break;
        case MEM_FILE:
        case NONE:
            ERROR("Can't store file type %i!\n", env.cfg.output_filetype);
            break;
    }

    return 0;
}

int store_a()
{
    Pid_t i;
    char *fname = tagged_output_name("acc");
    FILE *fp = fopen(fname, "wt");
    log("IO", "Storing accelerations in %s\n", fname);
    myassert(fp != NULL, "Can't open file to write accelerations.");
    fprintf(fp, "%ld\n", (long int)env.n_particles);
    forall_particles(i) 
        fprintf(fp, "%.8g %.8g %.8g\n", (double)ax(i), (double)ay(i), (double)az(i));
        //fprintf(fp, "%.8g %.8g %.8g %.8g\n", ax(i), ay(i), az(i), DIST(ax(i), ay(i), az(i)));
    fclose(fp);

    return 0;
}

int store_rhoe()
{
    Pid_t i;
    char *fname = tagged_output_name("rhoe");
    FILE *fp = fopen(fname, "wt");
    log("IO", "Storing density estimates in %s\n", fname);
    myassert(fp != NULL, "Can't open file to write density estimates.");
    fprintf(fp, "%ld\n", (long int)env.n_particles);
    forall_particles(i) 
        fprintf(fp, "%.8g\n", rhoe(i));
    fclose(fp);

    return 0;
}
