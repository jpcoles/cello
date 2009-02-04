#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "mem.h"
#include "jpeglib.h"

typedef struct
{
    int a,b;
} ab_t;


int icmp(const void *a0, const void *b0)
{
    const ab_t *a = (ab_t *)a0;
    const ab_t *b = (ab_t *)b0;

    if (a->a == b->a 
    ||  a->a == b->b
    ||  a->b == b->a
    ||  a->b == b->b) return 0;

    return a->a - a->b;
}

int main(int argc, char **argv)
{
    FILE *in, *out;
    int i;
    int x, y;
    int64_t max_id=0;


    ab_t *list = NULL;
    int list_size = 0, list_len = 0;

    if (argc < 2) 
    {
        fprintf(stderr, "Usage: isort <interaction-list> [output]\n");
        exit(2);
    }

    if ((in = fopen(argv[1], "r")) == NULL)
    {
        fprintf(stderr, "Can't open %s\n", argv[1]);
        exit(1);
    }

    if (argc == 3)
    {
        if ((out = fopen(argv[2], "wb")) == NULL) {
            fprintf(stderr, "Can't open %s\n", argv[2]);
            exit(1);
        }
    }
    else
    {
        char *filename = NULL;
        if (filename == NULL)
            filename = MALLOC(char, strlen(argv[1])+4+1);

        sprintf(filename, "%s.sorted", argv[1]);
        if ((out = fopen(filename, "wb")) == NULL) {
            fprintf(stderr, "Can't open %s\n", filename);
            exit(1);
        }

    }

    while (!feof(in))
    {
        fscanf(in, "%i %i\n", &x, &y);

        if (x > max_id) max_id = x;
        if (y > max_id) max_id = y;

        if (list_len == list_size)
        {
            if (list_size == 0) list_size = 2048;
            else list_size *= 2;
            list = REALLOC(list, ab_t, list_size);
            assert(list != NULL);
        }

        list[list_len].a = x;
        list[list_len].b = y;

        list_len++;
    }

    fclose(in);

    qsort(list, list_len, sizeof(ab_t), icmp);

    for (i=0; i < list_len; i++)
        fprintf(out, "%i %i\n", list[i].a, list[i].b);

    fclose(out);
    

    return 0;
}

