#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "mem.h"
#include "macros.h"
#include "jpeglib.h"

#define MAX(x,y) ((x) > (y) ? (x) : (y))

typedef struct
{
    uint32_t a,b;
    int sort_index;
} ab_t;

typedef struct
{
    uint32_t id, sort_index;
    int n;
} freq_t;

static freq_t *freq = NULL;


int icmp(const void *a0, const void *b0)
{
    const ab_t *a = (ab_t *)a0;
    const ab_t *b = (ab_t *)b0;

    return a->sort_index - b->sort_index;
}

int freq_cmp(const void *a0, const void *b0)
{
    const freq_t *a = (freq_t *)a0;
    const freq_t *b = (freq_t *)b0;

    return a->n - b->n;
}

int main(int argc, char **argv)
{
    FILE *in, *out;
    uint32_t i;
    uint32_t x, y;
    uint32_t max_id=0;


    ab_t *list = NULL;
    int list_size = 0, list_len = 0;

    if (argc < 2) 
    {
        fprintf(stderr, "Usage: isort2 <interaction-list> [output]\n");
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

        myassert(x != y, "Nodes can't interact with themselves!");
        
        list_len++;
    }

    fclose(in);


    fprintf(stderr, "max_id: %i\n", max_id);

             freq  = MALLOC(freq_t, max_id+1); assert(freq  != NULL);
    freq_t *sorted_freq = MALLOC(freq_t, max_id+1); assert(sorted_freq != NULL);

    uint32_t j;

    for (i=1; i <= max_id; i++)
    {
        freq[i].id = i;
        freq[i].n  = 0;
    }

    for (i=0; i < list_len; i++)
    {
        freq[list[i].a].n++;
        freq[list[i].b].n++;
    }

    memcpy(sorted_freq, freq, sizeof(freq_t) * (max_id + 1));

    qsort(sorted_freq+1, max_id, sizeof(freq_t), freq_cmp);

    uint32_t freq_sum=0;
    for (j=1; j <= max_id; j++) freq_sum += freq[ j ].n;
    assert(freq_sum == list_len);

    for (j=1; j <= max_id; j++)
        freq[ sorted_freq[j].id ].sort_index = j;

    for (i=0; i < list_len; i++)
        list[i].sort_index = MAX( freq[list[i].a].sort_index, freq[list[i].b].sort_index );

    qsort(list, list_len, sizeof(ab_t), icmp);

    for (i=1; i < list_len; i++)
        assert(list[i].sort_index >= list[i-1].sort_index);

    for (i=0; i < list_len; i++)
    {
        if (freq[list[i].a].sort_index < freq[list[i].b].sort_index)
            fprintf(out, "%i %i %i %i\n", freq[list[i].b].sort_index, freq[list[i].b].n, list[i].a, list[i].b);
        else
            fprintf(out, "%i %i %i %i\n", freq[list[i].a].sort_index, freq[list[i].a].n, list[i].b, list[i].a);
    }

    fclose(out);
    

    return 0;
}

