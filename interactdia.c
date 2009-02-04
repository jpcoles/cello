#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <jpeglib.h>
#include <png.h>
#include "mem.h"

void color_ramp_hot2cold(float *r, float *g, float *b);
int save_image(char *filename, unsigned char *image, uint64_t size);
int save_image_png(char *filename, unsigned char *image, uint64_t size);
int save_image_jpg(char *filename, unsigned char *image, uint64_t size);
void help();


void help()
{
    fprintf(stderr, "Usage: interactdia [--multi|-m][=every] [-o imagefile] <interaction-list>\n");
    exit(2);
}

void color_ramp_hot2cold(float *r, float *g, float *b)
{
    const float r0 = *r;
    const float g0 = *g;
    const float b0 = *b;

    float cr, cg, cb;
    cr = cg = cb = 1.0F;

    float v = r0;
    //float v = sqrtf(r0*r0 + g0*g0 + b0*b0);

    if (v < 0.0F)
      v = 0;
    if (v > 1.0F)
      v = 1.0F;

    if (v < 0.25) {
      cr = 0;
      cg = 4 * v;
    } else if (v < (0.5)) {
      cr = 0;
      cb = 1 + 4 * (0.25 - v);
    } else if (v < (0.75)) {
      cr = 4 * (v - 0.5);
      cb = 0;
    } else {
      cg = 1 + 4 * (0.75 - v);
      cb = 0;
    }

    *r = cr;
    *g = cg;
    *b = cb;

}

int save_image(char *filename, unsigned char *image, uint64_t size)
{
    return save_image_png(filename, image, size);
}

int save_image_png(char *filename, unsigned char *image, uint64_t size)
{
    int i;

    char *fname = MALLOC(char, strlen(filename)+4+1);
    sprintf(fname, "%s.png", filename);

    FILE *fp = fopen(fname, "wb");

    if (fp == NULL)
    {
        fprintf(stderr, "Can't open %s\n", fname);
        FREE(fname);
        return 1;
    }
    FREE(fname);

    png_structp png_ptr = png_create_write_struct
       (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) return 1;

    png_init_io(png_ptr, fp);

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
       png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
       return 1;
    }


    png_set_IHDR(png_ptr, info_ptr, size, size,
           8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
           PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);

    int row_stride = size * 3;

    for (i=0; i < size; i++)
    {
        png_bytep row_pointer = & (image[i * row_stride]);
        png_write_row(png_ptr, row_pointer);
    }

    png_write_end(png_ptr, info_ptr);

    png_destroy_write_struct(&png_ptr, &info_ptr);

    fclose(fp);

    return 0;
}

int save_image_jpg(char *filename, unsigned char *image, uint64_t size)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    char *fname = MALLOC(char, strlen(filename)+4+1);
    sprintf(fname, "%s.jpg", filename);

    FILE *out = fopen(fname, "wb");

    if (out == NULL)
    {
        fprintf(stderr, "Can't open %s\n", fname);
        FREE(fname);
        return 1;
    }
    FREE(fname);

    fprintf(stderr, "Writing %s\n", filename);

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    jpeg_stdio_dest(&cinfo, out);

    cinfo.image_width      = size;
    cinfo.image_height     = size;
    cinfo.input_components = 3;
    cinfo.in_color_space   = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_start_compress(&cinfo, TRUE);

    JSAMPROW row_pointer[1];              /* pointer to a single row */
    int row_stride;                       /* physical row width in buffer */

    row_stride = size * 3;   /* JSAMPLEs per row in image_buffer */

    while (cinfo.next_scanline < cinfo.image_height) 
    {
        //row_pointer[0] = & (image[(cinfo.image_height-cinfo.next_scanline-1) * row_stride]);
        row_pointer[0] = & (image[cinfo.next_scanline * row_stride]);
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    fclose(out);

    return 0;
}

int main(int argc, char **argv)
{
    FILE *in;
    int i;
    int x, y;
    int64_t max_id=0;

    char *outfile = NULL;
    char *infile = NULL;

    typedef struct
    {
        int a,b;
    } ab_t;

    ab_t *list = NULL;
    int list_size = 0, list_len = 0;
    int multi = 0;
    int every=1;
    int size=400;
    int max=0;

    /*========================================================================
     * Process the command line flags
     *======================================================================*/
    if (argc < 2) help();

    while (1)
    {
        static struct option long_options[] = {
            {"help", no_argument, 0, 'h'},
            {"multi", optional_argument, 0, 'm'},
            {"size", required_argument, 0, 's'},
            {"max", required_argument, 0, 'M'},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "o:m::s:",
                            long_options, &option_index);

        if (c == -1) break;

        switch (c)
        {
            case 'o': outfile = optarg; break;
            case 's': size = atoi(optarg); break;
            case 'm': 
                multi = 1;
                if (optarg) multi = atoi(optarg);
                if (multi < 1) help();
                break;
            case 'M':
                max = atoi(optarg);
                if (max < 1) help();
                break;
            case 'h': help(); break;
            case '?': break;
        }
    }

    if (optind >= argc) help();

    infile = argv[optind];
    if ((in = fopen(infile, "r")) == NULL)
    {
        fprintf(stderr, "Can't open %s\n", argv[optind]);
        exit(1);
    }

#if 0

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

        sprintf(filename, "%s.jpg", argv[1]);
        if ((out = fopen(filename, "wb")) == NULL) {
            fprintf(stderr, "Can't open %s\n", filename);
            exit(1);
        }

    }
#endif
    char *line = NULL;
    size_t len; 
    ssize_t read;


    fprintf(stderr, "Reading interaction list...\n");
    //
    // We need to read the list first, just so we know the maximum id
    //
    while ((read = getline(&line, &len, in)) != -1) 
    {
        sscanf(line, "%i %i\n", &x, &y);

        if (x < y) { int t = x; x = y; y = t; }

        //assert(y <= x);

        if (max && (x > max || y > max)) continue;

        x -= 1;
        y -= 1;

        if (x > max_id) max_id = x;
        if (y > max_id) max_id = y;

        if (list_len == list_size)
        {
            if (list_size == 0) list_size = 2048;
            else list_size *= 2;
            list = REALLOC(list, ab_t, list_size);
            assert(list != NULL);
        }

        list[list_len].a = y;
        list[list_len].b = x;

        list_len++;
    }

    fclose(in);

    fprintf(stderr, "Done.\n");

    float scale = (float)size / (max_id+1);

    fprintf(stderr, "Allocating %ld bytes\n", size*size*3*sizeof(unsigned char));
    unsigned char *image = CALLOC(unsigned char, size*size*3*sizeof(unsigned char));
    assert(image != NULL);

    int image_index=0;
    char *filename = outfile;
    if (multi)
    {
        filename = MALLOC(char, strlen(outfile)+7+4+1);
        every = list_len / multi;
        if (every == 0) every = 1;
    }
    else
    {
        if (filename == NULL)
            filename = infile;
    }


    fprintf(stderr, "max_id=%li\n", max_id);
    fprintf(stderr, "scale=%f\n", scale);
    fprintf(stderr, "size=%i\n", size);

    for (i=0; i < list_len; i++)
    {
        int64_t y = (int64_t)(list[i].a*scale);
        int64_t x = (int64_t)(list[i].b*scale);
        //if (x >= size) x = size-1;
        //if (y >= size) y = size-1;

        int64_t offs = 3*(y*size + x);
        //printf("%i %i, %ld\n", list[i].a, list[i].b, offs);
        //printf("%li %li, %ld\n", y, x, offs);
        //assert(y <= x);
        float r, g, b;
        r = g = b = (i+1.0) / list_len;

        color_ramp_hot2cold(&r, &g, &b);
        image[offs+0] = (unsigned char)255*r;
        image[offs+1] = (unsigned char)255*g;
        image[offs+2] = (unsigned char)255*b;

        if (multi && ((i % every) == 0))
        {
            sprintf(filename, "%s.%06i", outfile, image_index++);
            if (save_image(filename, image, size)) exit(1);
        }
    }

    if (multi && ((i % every) == 0))
        sprintf(filename, "%s.%06i", outfile, image_index++);

    if (save_image(filename, image, size)) exit(1);

    return 0;
}

