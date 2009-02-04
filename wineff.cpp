#include <set>
#include <stdio.h>
#include <math.h> // min,max

using namespace std;

#define N_WINDOW_SIZES  12L
#define MIN_WINDOW_SIZE (1L << 8)

typedef struct
{
    int size;
    set<int> window;

    //float eff, min_eff, max_eff;
    float eff, min_eff, max_eff;
    float *all_eff;
    int n;

    int lines;

} window_t;

float transfer_time(uint64_t win_size)
{
    const float bytes_per_node = 232;
    const float bus_speed = 2e9;
    return bytes_per_node * win_size / bus_speed;
}

float compute_time(float avg_n_interactions)
{
    const float MOMR_op = 800;
    const float gpu_speed = 3.45e11;
    return avg_n_interactions * MOMR_op / gpu_speed;
    //return (win_sizes**2) * MOMR_op / gpu_speed;
}

int main(int argc, char **argv)
{
    FILE *in, *plot_out, *win_eff_out;
    int x, y;
    int total_lines = 0;

    // If the total number of lines in the file is given, then
    // we print the percentage complete on stderr.
    if (argc < 5) 
    { 
        fprintf(stderr, "Usage: file no.lines file_out file_out2\n");
        exit(2); 
    }

    if ((in = fopen(argv[1], "r")) == NULL)
    {
        fprintf(stderr, "Can't open %s\n", argv[1]);
        exit(2);
    }

    total_lines = atoi(argv[2]);

    if ((plot_out = fopen(argv[3], "w")) == NULL)
    {
        fprintf(stderr, "Can't open %s\n", argv[3]);
        exit(2);
    }

    if ((win_eff_out = fopen(argv[4], "w")) == NULL)
    {
        fprintf(stderr, "Can't open %s\n", argv[4]);
        exit(2);
    }

    //------------------------------------------------------------------------
    // Initialize bins and set structures
    //------------------------------------------------------------------------
    window_t win[N_WINDOW_SIZES];
    for (int i=0; i < N_WINDOW_SIZES; i++)
    {
        win[i].size = MIN_WINDOW_SIZE << i;
        win[i].n = 0;
        win[i].eff = 0;
        win[i].all_eff = (float *)calloc(1000, sizeof(float));
        win[i].lines = 0;
        win[i].min_eff = 1e10;
        win[i].max_eff = -1e10;
    }

    int max_id=0;

    int lineno=0;
    while (!feof(in))
    {
        int32_t dummy;

        fscanf(in, "%i %i %i\n", &x, &y, &dummy);

        if (x > max_id) max_id = x;
        if (y > max_id) max_id = y;

        if (++lineno % 100000 == 0) 
        { 
            if (total_lines == 0) fprintf(stderr, "."); 
            else                  fprintf(stderr, "\r%i%%", (int)(100.0 * lineno / total_lines));
            fflush(stderr); 
        }

        //--------------------------------------------------------------------
        // For each window size we add the current line to the window set
        // If we completely fill the window with unique items then 
        // calculate the efficiency and prepare a new window.
        //
        // Efficiency is defined as the ratio of the number of lines that can 
        // be packed into a given window size to the window size.
        //--------------------------------------------------------------------
        for (int i=0; i < N_WINDOW_SIZES; i++)
        {
            win[i].window.insert(x);
            win[i].window.insert(y);
            win[i].lines++;

            int size = win[i].window.size();

            if (size >= win[i].size)
            {
                float yval = ((float)win[i].lines) / size;
                win[i].all_eff[win[i].n] = yval;
                win[i].eff += yval;
                win[i].n++;
                win[i].min_eff = fmin(win[i].min_eff, yval);
                win[i].max_eff = fmax(win[i].max_eff, yval);
                win[i].window.clear();
                win[i].lines = 0;
            }
        }

    }

    fclose(in);

    fprintf(stderr, "\n");

#if 0
    for (int i=0; i < N_WINDOW_SIZES; i++)
    {
        if (!win[i].window.empty())
        {
            float yval = ((float)win[i].lines) / win[i].window.size();
            //float yval = ((float)win[i].lines) / win[i].size;
            win[i].eff += yval;
            win[i].n++;
            win[i].min_eff = fmin(win[i].min_eff, yval);
            win[i].max_eff = fmax(win[i].max_eff, yval);
        }
    }
#endif
         
    for (int i=0; i < N_WINDOW_SIZES; i++)
    {
        if (win[i].n == 0)
        {
            fprintf(plot_out, 
                   "%9i %9i %9i % 8.4f % 8.4f % 8.4f  % 8.4f % 8.4f  % 8.4f\n",
                win[i].size, max_id, win[i].n, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
        }
        else
        {
            fprintf(plot_out, 
                   "%9i %9i %9i % 8.4f % 8.4f % 8.4f  % 8.4f % 8.4f  % 8.4f\n",
                   win[i].size, max_id, win[i].n, (win[i].eff / win[i].n),
                   win[i].min_eff, win[i].max_eff,
                   compute_time(float(lineno) / win[i].n), transfer_time(win[i].size),
                   compute_time(float(lineno) / win[i].n) / transfer_time(win[i].size));
        }

        for (int j=0; j < win[i].n; j++)
            fprintf(win_eff_out, "% 8i % 8.4f\n", i, win[i].all_eff[j]);
        fprintf(win_eff_out, "\n");
    }

    fclose(plot_out);
    fclose(win_eff_out);

    return 0;
}

