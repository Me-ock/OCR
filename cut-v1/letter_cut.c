#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void save_subimage(unsigned char *img, int width, int height,
                          int x0, int y0, int x1, int y1, const char *filename)
{
    if (x1 <= x0 || y1 <= y0) return;
    int w = x1 - x0 + 1;
    int h = y1 - y0 + 1;
    unsigned char *sub = malloc(w * h);
    if (!sub) return;
    for (int y = 0; y < h; y++)
        memcpy(sub + y * w, img + (y + y0) * width + x0, w);
    stbi_write_png(filename, w, h, 1, sub, w);
    free(sub);
}

static int detect_runs(int *proj, int len, int threshold, int *runs, int maxruns)
{
    int in_zone = 0, idx = 0;
    int start = 0;
    for (int i = 0; i < len; i++) {
        if (proj[i] > threshold) {
            if (!in_zone) { in_zone = 1; start = i; }
        } else {
            if (in_zone) {
                if (idx + 2 <= maxruns) {
                    runs[idx++] = start;
                    runs[idx++] = i - 1;
                }
                in_zone = 0;
            }
        }
    }
    if (in_zone) {
        if (idx + 2 <= maxruns) {
            runs[idx++] = start;
            runs[idx++] = len - 1;
        }
    }
    return idx;
}

int main(int argc, char **argv)
{
    if (argc < 2) { printf("Usage: %s grid.png\n", argv[0]); return 1; }

    int w, h, c;
    unsigned char *img = stbi_load(argv[1], &w, &h, &c, 1);
    if (!img) { printf("Error: cannot load %s\n", argv[1]); return 1; }

    // projections
    int *proj_x = calloc(w, sizeof(int));
    int *proj_y = calloc(h, sizeof(int));
    if (!proj_x || !proj_y) { printf("Memory error\n"); return 1; }

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (img[y * w + x] < 128) { proj_x[x]++; proj_y[y]++; }
        }
    }

    // thresholds for detecting grid lines: adapt to image
    int max_px = 0, max_py = 0;
    for (int i = 0; i < w; i++) if (proj_x[i] > max_px) max_px = proj_x[i];
    for (int i = 0; i < h; i++) if (proj_y[i] > max_py) max_py = proj_y[i];

    // choose a threshold that catches the grid lines (which span almost full image)
    int thr_x = (int)(0.6 * (double)h);
    if ((int)(0.6 * (double)max_px) > thr_x) thr_x = (int)(0.6 * (double)max_px);

    int thr_y = (int)(0.6 * (double)w);
    if ((int)(0.6 * (double)max_py) > thr_y) thr_y = (int)(0.6 * (double)max_py);

    // arrays to store runs
    int *runs_x = malloc(sizeof(int) * w * 2);
    int *runs_y = malloc(sizeof(int) * h * 2);
    int cnt_x = detect_runs(proj_x, w, thr_x, runs_x, w*2);
    int cnt_y = detect_runs(proj_y, h, thr_y, runs_y, h*2);

    int num_lines_x = cnt_x / 2;
    int num_lines_y = cnt_y / 2;

    printf("detected vertical runs: %d  horizontal runs: %d  (thr_x=%d thr_y=%d)\n",
           num_lines_x, num_lines_y, thr_x, thr_y);

    // If we have enough runs (>=2) in both directions, we assume grid has drawn lines
    if (num_lines_x >= 2 && num_lines_y >= 2) {
        // compute centers of runs
        int *centers_x = malloc(sizeof(int) * num_lines_x);
        int *centers_y = malloc(sizeof(int) * num_lines_y);
        for (int i = 0; i < num_lines_x; i++) {
            int s = runs_x[2*i], e = runs_x[2*i+1];
            centers_x[i] = (s + e) / 2;
        }
        for (int i = 0; i < num_lines_y; i++) {
            int s = runs_y[2*i], e = runs_y[2*i+1];
            centers_y[i] = (s + e) / 2;
        }

        // number of cells = lines - 1
        int cols = num_lines_x - 1;
        int rows = num_lines_y - 1;
        if (cols <= 0 || rows <= 0) {
            printf("Not enough grid lines to form cells\n");
        } else {
            printf("Grid lines detected -> %d rows x %d cols\n", rows, cols);
            system("mkdir -p letters");
            int idx = 0;
            for (int r = 0; r < rows; r++) {
                for (int cidx = 0; cidx < cols; cidx++) {
                    int x0 = centers_x[cidx] + 1;
                    int x1 = centers_x[cidx+1] - 1;
                    int y0 = centers_y[r] + 1;
                    int y1 = centers_y[r+1] - 1;
                    // sanity clamp
                    if (x0 < 0) x0 = 0;
                    if (y0 < 0) y0 = 0;
                    if (x1 >= w) x1 = w-1;
                    if (y1 >= h) y1 = h-1;
                    char name[256];
                    sprintf(name, "letters/letter_%02d_%02d.png", r, cidx);
                    save_subimage(img, w, h, x0, y0, x1, y1, name);
                    idx++;
                }
            }
            printf("%d letters saved into folder letters/\n", idx);
        }
        free(centers_x);
        free(centers_y);
    } else {
        // fallback: try the previous projection-based zone detection (less reliable if no lines)
        printf("Fallback: no strong grid lines detected, trying projection-based splitting\n");
        // detect continuous areas in proj arrays (same logic but using MIN_RUN)
        int MIN_RUN = 2;
        int *zones_x = malloc(sizeof(int) * w * 2);
        int *zones_y = malloc(sizeof(int) * h * 2);
        int count_x = 0, count_y = 0;
        // detect_x
        {
            int in_zone = 0, idx = 0;
            for (int i = 0; i < w; i++) {
                if (proj_x[i] > MIN_RUN && !in_zone) { in_zone = 1; zones_x[idx++] = i; }
                else if (proj_x[i] <= MIN_RUN && in_zone) { in_zone = 0; zones_x[idx++] = i-1; }
            }
            if (in_zone) zones_x[idx++] = w-1;
            count_x = idx;
        }
        // detect_y
        {
            int in_zone = 0, idx = 0;
            for (int i = 0; i < h; i++) {
                if (proj_y[i] > MIN_RUN && !in_zone) { in_zone = 1; zones_y[idx++] = i; }
                else if (proj_y[i] <= MIN_RUN && in_zone) { in_zone = 0; zones_y[idx++] = i-1; }
            }
            if (in_zone) zones_y[idx++] = h-1;
            count_y = idx;
        }
        if (count_x >= 4 && count_y >= 4) {
            int cols = count_x/2;
            int rows = count_y/2;
            printf("Projection fallback: rows=%d cols=%d\n", rows, cols);
            system("mkdir -p letters");
            int idx = 0;
            for (int r = 0; r < count_y; r += 2) {
                for (int cidx = 0; cidx < count_x; cidx += 2) {
                    int x0 = zones_x[cidx];
                    int x1 = zones_x[cidx+1];
                    int y0 = zones_y[r];
                    int y1 = zones_y[r+1];
                    // shrink a bit to avoid grid lines
                    x0 += 1; y0 += 1; if (x1 < w) x1 -= 1; if (y1 < h) y1 -= 1;
                    char name[256];
                    sprintf(name, "letters/letter_%02d_%02d.png", r/2, cidx/2);
                    save_subimage(img, w, h, x0, y0, x1, y1, name);
                    idx++;
                }
            }
            printf("%d letters saved into folder letters/ (fallback)\n", idx);
        } else {
            printf("Fallback also failed: cannot find a regular grid.\n");
        }
        free(zones_x);
        free(zones_y);
    }

    free(runs_x);
    free(runs_y);
    free(proj_x);
    free(proj_y);
    stbi_image_free(img);
    return 0;
}

