#include <stdlib.h>
#include "preprocessing.h"
#include <math.h>
#include <string.h>

// met en gris pour que ce soit plus simple, OCR pas besoin de couleur
Image* to_grayscale(Image *src) 
{
    if (!src || !src->data) return NULL;

    Image *gray = malloc(sizeof(Image));
    if (!gray) return NULL;

    gray->width = src->width;
    gray->height = src->height;
    gray->channels = 1; // couche, besoin que d'1 seul
    gray->data = malloc(gray->width * gray->height);
    if (!gray->data) {
        free(gray);
        return NULL;
    }

    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < src->width; x++) {
            int idx = (y * src->width + x) * src->channels;
            unsigned char r = src->data[idx + 0];
            unsigned char g = src->data[idx + 1];
            unsigned char b = src->data[idx + 2];
            gray->data[y * gray->width + x] = (unsigned char)(0.3*r + 0.59*g + 0.11*b);
        }
    }
    return gray;
}


// permet le stockage  
Image* to_binary(Image *src, unsigned char threshold) 
{
    if (!src || !src->data) return NULL;

    Image *bin = malloc(sizeof(Image));
    if (!bin) return NULL;

    bin->width = src->width;
    bin->height = src->height;
    bin->channels = 1;  // noir et blanc
    bin->data = malloc(bin->width * bin->height);
    if (!bin->data) {
        free(bin);
        return NULL;
    }

    for (int i = 0; i < src->width * src->height; i++) {
        bin->data[i] = (src->data[i] > threshold) ? 255 : 0;
    }

    return bin;
}

static unsigned char otsu_threshold(Image *src)
{
    if (!src || !src->data || src->channels != 1)
        return 128;

    int w = src->width;
    int h = src->height;
    int N = w * h;

    int hist[256] = {0};

    for (int i = 0; i < N; ++i) {
        unsigned char v = src->data[i];
        hist[v]++;
    }

    // somme totale du grisage
    double sum = 0.0;
    for (int t = 0; t < 256; ++t)
        sum += (double)t * hist[t];

    double sumB = 0.0;
    int wB = 0;
    int wF = 0;
    double maxVar = -1.0;
    int bestT = 128;

    for (int t = 0; t < 256; ++t) {
        wB += hist[t];
        if (wB == 0)
            continue;
        wF = N - wB;
        if (wF == 0)
            break;

        sumB += (double)t * hist[t];

        double mB = sumB / wB;
        double mF = (sum - sumB) / wF;

        double varBetween = (double)wB * (double)wF * (mB - mF) * (mB - mF);

        if (varBetween > maxVar) {
            maxVar = varBetween;
            bestT = t;
        }
    }

    return (unsigned char)bestT;
}

Image* to_binary_auto(Image *src)
{
    if (!src || !src->data || src->channels != 1)
        return NULL;

    unsigned char thr = otsu_threshold(src);
    return to_binary(src, thr);
}

static Image* copy_image_1c(const Image *src)
{
    if (!src || !src->data || src->channels != 1) return NULL;
    size_t N = (size_t)src->width * src->height;

    Image *dst = malloc(sizeof(Image));
    if (!dst) return NULL;
    dst->width = src->width; dst->height = src->height; dst->channels = 1;
    dst->data = malloc(N);
    if (!dst->data) { free(dst); return NULL; }

    memcpy(dst->data, src->data, N);
    return dst;
}

static Image* downscale_half_1c(const Image *src)
{
    if (!src || !src->data || src->channels != 1) return NULL;
    int nw = src->width / 2;  if (nw < 1) nw = 1;
    int nh = src->height/ 2;  if (nh < 1) nh = 1;

    Image *dst = malloc(sizeof(Image));
    if (!dst) return NULL;
    dst->width = nw; dst->height = nh; dst->channels = 1;
    dst->data = malloc((size_t)nw * nh);
    if (!dst->data) { free(dst); return NULL; }

    for (int y = 0; y < nh; ++y)
        for (int x = 0; x < nw; ++x)
            dst->data[y*nw + x] = src->data[(y*2)*src->width + (x*2)];

    return dst;
}

Image* denoise_image_median3x3(Image *src)
{
    if (!src || !src->data || src->channels != 1)
        return NULL;

    int w = src->width;
    int h = src->height;

    Image *dst = malloc(sizeof(Image));
    if (!dst) return NULL;
    dst->width = w;
    dst->height = h;
    dst->channels = 1;
    dst->data = malloc((size_t)w*h);
    if (!dst->data) { free(dst); return NULL; }

    memcpy(dst->data, src->data, (size_t)w*h);

    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            if (src->data[y*w + x] != 0)
                continue;

            int black = 0;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dy == 0 && dx == 0)
                        continue;
                    if (src->data[(y+dy)*w + (x+dx)] == 0)
                        black++;
                }
            }

            if (black == 0)
                dst->data[y*w + x] = 255;
        }
    }

    return dst;
}


// variance des projections colonne (verticale)
static double var_proj_x(const Image *im)
{
    int w = im->width, h = im->height;
    double *col = malloc(sizeof(double) * (size_t)w);
    if (!col) return 0.0;

    for (int x = 0; x < w; ++x) {
        int sum = 0;
        for (int y = 0; y < h; ++y)
            sum += (im->data[y*w + x] < 128); // noir
        col[x] = (double)sum;
    }
    double mean = 0.0;
    for (int x = 0; x < w; ++x) mean += col[x];
    mean /= (double)w;

    double var = 0.0;
    for (int x = 0; x < w; ++x) {
        double d = col[x] - mean; var += d*d;
    }
    var /= (double)w;
    free(col);
    return var;
}

// variance des projections ligne (horizontale)
static double var_proj_y(const Image *im)
{
    int w = im->width, h = im->height;
    double *row = malloc(sizeof(double) * (size_t)h);
    if (!row) return 0.0;

    for (int y = 0; y < h; ++y) {
        int sum = 0;
        for (int x = 0; x < w; ++x)
            sum += (im->data[y*w + x] < 128);
        row[y] = (double)sum;
    }
    double mean = 0.0;
    for (int y = 0; y < h; ++y) mean += row[y];
    mean /= (double)h;

    double var = 0.0;
    for (int y = 0; y < h; ++y) {
        double d = row[y] - mean; var += d*d;
    }
    var /= (double)h;
    free(row);
    return var;
}

// rotation bilinéaire 1 canal
static Image* rotate_bilinear_1c(const Image *src, float angle_deg)
{
    if (!src || !src->data || src->channels != 1) return NULL;

    float a = angle_deg * (float)M_PI / 180.0f;
    float ca = cosf(a), sa = sinf(a);
    int w = src->width, h = src->height;

    int new_w = (int)(fabsf(w * ca) + fabsf(h * sa) + 0.5f);
    int new_h = (int)(fabsf(w * sa) + fabsf(h * ca) + 0.5f);
    if (new_w < 1) new_w = 1;
    if (new_h < 1) new_h = 1;

    Image *dst = malloc(sizeof(Image));
    if (!dst) return NULL;
    dst->width = new_w; dst->height = new_h; dst->channels = 1;
    dst->data = malloc((size_t)new_w * new_h);
    if (!dst->data) { free(dst); return NULL; }
    memset(dst->data, 255, (size_t)new_w * new_h); // fond blanc

    float cx  = (w  - 1) * 0.5f, cy  = (h  - 1) * 0.5f;
    float ncx = (new_w - 1) * 0.5f, ncy = (new_h - 1) * 0.5f;

    for (int y = 0; y < new_h; ++y) {
        for (int x = 0; x < new_w; ++x) {
            float xr =  (x - ncx) * ca + (y - ncy) * sa + cx;
            float yr = -(x - ncx) * sa + (y - ncy) * ca + cy;

            int x0 = (int)floorf(xr), y0 = (int)floorf(yr);
            float dx = xr - x0, dy = yr - y0;

            if (x0 >= 0 && x0 + 1 < w && y0 >= 0 && y0 + 1 < h) {
                unsigned char p00 = src->data[y0    * w + x0    ];
                unsigned char p01 = src->data[y0    * w + x0 + 1];
                unsigned char p10 = src->data[(y0+1)* w + x0    ];
                unsigned char p11 = src->data[(y0+1)* w + x0 + 1];

                float v0 = p00 * (1.0f - dx) + p01 * dx;
                float v1 = p10 * (1.0f - dx) + p11 * dx;
                float v  = v0  * (1.0f - dy) + v1  * dy;

		int vi = (int)(v + 0.5f);
		if (vi < 0)
			vi = 0;
		else if (vi > 255)
    			vi = 255;

		dst->data[y * new_w + x] = (unsigned char)vi;
            }
        }
    }
    return dst;
}

// vertical + horizontal
static double grid_score(const Image *bin)
{
    return var_proj_x(bin) + var_proj_y(bin);
}

static float estimate_skew_angle(Image *src_1c)
{
    const float min_deg  = -45.0f;
    const float max_deg  =  45.0f;
    const float step_deg =   0.5f;

    if (!src_1c || !src_1c->data || src_1c->channels != 1)
        return 0.0f;

    Image *small_gray = downscale_half_1c(src_1c);
    if (!small_gray)
        return 0.0f;

    unsigned char thr = otsu_threshold(small_gray);

    Image *small_bin = to_binary(small_gray, thr);
    free_image(small_gray);
    if (!small_bin)
        return 0.0f;

    double best_score = -1.0;
    float  best_a     =  0.0f;

    for (float a = min_deg; a <= max_deg + 1e-3f; a += step_deg) {
        Image *rot = rotate_bilinear_1c(small_bin, a);
        if (!rot)
            continue;

        double s = grid_score(rot);
        if (s > best_score) {
            best_score = s;
            best_a = a;
        }
        free_image(rot);
    }

    free_image(small_bin);
    return best_a;
}

Image* straighten_grid(Image *src)
{
    if (!src || !src->data || src->channels != 1)
        return NULL;

    float a = estimate_skew_angle(src);
    return rotate_bilinear_1c(src, a);
}

Image* remove_small_components(Image *src, int min_size)
{
    if (!src || !src->data || src->channels != 1)
        return NULL;

    int w = src->width;
    int h = src->height;
    size_t N = (size_t)w * h;

    Image *dst = malloc(sizeof(Image));
    if (!dst) return NULL;
    dst->width = w;
    dst->height = h;
    dst->channels = 1;
    dst->data = malloc(N);
    if (!dst->data) { free(dst); return NULL; }

    memcpy(dst->data, src->data, N);

    unsigned char *vis = calloc(N, 1);
    if (!vis) { free(dst->data); free(dst); return NULL; }

    int *queue = malloc(N * sizeof(int));
    if (!queue) { free(vis); free(dst->data); free(dst); return NULL; }

    for (size_t start = 0; start < N; ++start) {
        if (dst->data[start] != 0 || vis[start])
            continue;

        int head = 0, tail = 0;
        int count = 0;

        queue[tail++] = (int)start;
        vis[start] = 1;

        while (head < tail) {
            int idx = queue[head++];
            count++;

            int y = idx / w;
            int x = idx % w;

            if (x > 0) {
                int n = idx - 1;
                if (!vis[n] && dst->data[n] == 0) {
                    vis[n] = 1;
                    queue[tail++] = n;
                }
            }
            if (x + 1 < w) {
                int n = idx + 1;
                if (!vis[n] && dst->data[n] == 0) {
                    vis[n] = 1;
                    queue[tail++] = n;
                }
            }
            if (y > 0) {
                int n = idx - w;
                if (!vis[n] && dst->data[n] == 0) {
                    vis[n] = 1;
                    queue[tail++] = n;
                }
            }
            if (y + 1 < h) {
                int n = idx + w;
                if (!vis[n] && dst->data[n] == 0) {
                    vis[n] = 1;
                    queue[tail++] = n;
                }
            }
        }

        if (count < min_size) {
            for (int i = 0; i < tail; ++i)
                dst->data[queue[i]] = 255;
        }
    }

    free(queue);
    free(vis);
    return dst;
}


void remove_grid_lines(Image *bin)
{
    if (!bin || !bin->data || bin->channels != 1)
        return;

    int w = bin->width;
    int h = bin->height;
    int N = w * h;

    unsigned char *data = bin->data;

    unsigned char *visited = calloc(N, 1);
    if (!visited) return;

    int *qx = malloc(sizeof(int) * N);
    int *qy = malloc(sizeof(int) * N);
    int *comp = malloc(sizeof(int) * N); /* indices des pixels du composant */
    if (!qx || !qy || !comp) {
        free(visited);
        free(qx);
        free(qy);
        free(comp);
        return;
    }

    /* longueur minimale pour considérer une vraie ligne */
    int max_dim = (w > h) ? w : h;
    int min_len = max_dim / 6;          /* ~ un sixième de l'image */

    for (int y0 = 0; y0 < h; ++y0) {
        for (int x0 = 0; x0 < w; ++x0) {
            int idx0 = y0 * w + x0;
            if (data[idx0] != 0 || visited[idx0])
                continue;

            /* BFS sur le composant noir */
            int head = 0, tail = 0;
            qx[tail] = x0;
            qy[tail] = y0;
            tail++;
            visited[idx0] = 1;

            int count = 0;
            int minx = x0, maxx = x0;
            int miny = y0, maxy = y0;

            while (head < tail) {
                int x = qx[head];
                int y = qy[head];
                int idx = y * w + x;
                head++;

                comp[count++] = idx;

                if (x < minx) minx = x;
                if (x > maxx) maxx = x;
                if (y < miny) miny = y;
                if (y > maxy) maxy = y;

                /* 4-connexe */
                const int dx[4] = {1,-1,0,0};
                const int dy[4] = {0,0,1,-1};
                for (int k = 0; k < 4; ++k) {
                    int nx = x + dx[k];
                    int ny = y + dy[k];
                    if (nx < 0 || nx >= w || ny < 0 || ny >= h)
                        continue;
                    int nidx = ny * w + nx;
                    if (data[nidx] == 0 && !visited[nidx]) {
                        visited[nidx] = 1;
                        qx[tail] = nx;
                        qy[tail] = ny;
                        tail++;
                    }
                }
            }

            int width  = maxx - minx + 1;
            int height = maxy - miny + 1;

            int vertical = 0;
            int horizontal = 0;

            /* ligne verticale très fine et longue */
            if (width <= 3 && height >= min_len && height >= 4 * width)
                vertical = 1;

            /* ligne horizontale très fine et longue */
            if (height <= 3 && width >= min_len && width >= 4 * height)
                horizontal = 1;

            if (vertical || horizontal) {
                /* on efface ce composant = ligne de grille */
                for (int i = 0; i < count; ++i)
                    data[comp[i]] = 255;
            }
        }
    }

    free(visited);
    free(qx);
    free(qy);
    free(comp);
}

