#include <stdlib.h>
#include "preprocessing.h"
#include <math.h>

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

Image* rotate_image(Image *src, float angle_degrees) {
    if (!src || !src->data) return NULL;

    // Convertis en radian pour cos et sin
    float angle = angle_degrees * M_PI / 180.0f;
    float cos_a = cos(angle);
    float sin_a = sin(angle);

    int w = src->width;
    int h = src->height;

    // Calcule environ dimensions de l'image tournée
    int new_w = (int)(fabs(w * cos_a) + fabs(h * sin_a));
    int new_h = (int)(fabs(w * sin_a) + fabs(h * cos_a));

    Image *rot = malloc(sizeof(Image));
    if (!rot) return NULL;
    rot->width = new_w;
    rot->height = new_h;
    rot->channels = src->channels;
    rot->data = calloc(new_w * new_h * rot->channels, 1);
    if (!rot->data) { free(rot); return NULL; }

    // Trouver le centre des deux images
    float cx = w / 2.0f;
    float cy = h / 2.0f;
    float ncx = new_w / 2.0f;
    float ncy = new_h / 2.0f;

    // Parcour de chaque pixel de l'image résultat
    for (int y = 0; y < new_h; y++) {
        for (int x = 0; x < new_w; x++) {
            float xr =  (x - ncx) * cos_a + (y - ncy) * sin_a + cx;
            float yr = -(x - ncx) * sin_a + (y - ncy) * cos_a + cy;

            if (xr >= 0 && yr >= 0 && xr < w && yr < h) {
                int src_index = ((int)yr * w + (int)xr) * src->channels;
                int dst_index = (y * new_w + x) * rot->channels;

                for (int c = 0; c < rot->channels; c++) {
                    rot->data[dst_index + c] = src->data[src_index + c];
                }
            }
        }
    }

    return rot;
}

static void isort_9(unsigned char *a) {
    // tri insertion sur 9 éléments (rapide et suffisant)
    for (int i = 1; i < 9; ++i) {
        unsigned char key = a[i];
        int j = i - 1;
        while (j >= 0 && a[j] > key) { a[j+1] = a[j]; j--; }
        a[j+1] = key;
    }
}

Image* denoise_image_median3x3(Image *src) {
    if (!src || !src->data || src->channels != 1) return NULL;

    int w = src->width, h = src->height;
    Image *dst = malloc(sizeof(Image));
    if (!dst) return NULL;
    dst->width = w; dst->height = h; dst->channels = 1;
    dst->data = malloc((size_t)w * h);
    if (!dst->data) { free(dst); return NULL; }

    // bords: copie directe
    memcpy(dst->data, src->data, (size_t)w * h);

    // noyau 3x3
    unsigned char win[9];
    for (int y = 1; y < h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            int k = 0;
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                    win[k++] = src->data[(y+dy)*w + (x+dx)];
            isort_9(win);
            dst->data[y*w + x] = win[4]; // médiane
        }
    }
    return dst;
}

Image* normalize_contrast(Image *src) {
    if (!src || !src->data || src->channels != 1) return NULL;
    int w = src->width, h = src->height;
    const size_t N = (size_t)w * h;

    unsigned char minv = 255, maxv = 0;
    for (size_t i = 0; i < N; ++i) {
        unsigned char v = src->data[i];
        if (v < minv) minv = v;
        if (v > maxv) maxv = v;
    }
    // si plat, copie
    Image *dst = malloc(sizeof(Image));
    if (!dst) return NULL;
    dst->width = w; dst->height = h; dst->channels = 1;
    dst->data = malloc(N);
    if (!dst->data) { free(dst); return NULL; }

    if (maxv == minv) {
        memcpy(dst->data, src->data, N);
        return dst;
    }
    float scale = 255.0f / (float)(maxv - minv);
    for (size_t i = 0; i < N; ++i) {
        int v = (int)((src->data[i] - minv) * scale + 0.5f);
        if (v < 0) v = 0; if (v > 255) v = 255;
        dst->data[i] = (unsigned char)v;
    }
    return dst;
}

// Réduction rapide (nearest), 1 canal
static Image* downscale_half(Image *src) {
    if (!src || !src->data || src->channels != 1) return NULL;
    int nw = src->width  / 2; if (nw < 1) nw = 1;
    int nh = src->height / 2; if (nh < 1) nh = 1;
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

// Score = variance des projections verticales (plus c'est grand, plus les colonnes sont nettes)
static double vertical_projection_variance(Image *bin) {
    int w = bin->width, h = bin->height;
    // On compte les pixels noirs (0) par colonne → plus c’est structuré, plus la variance varie
    double *col = malloc(sizeof(double) * (size_t)w);
    if (!col) return 0.0;
    for (int x = 0; x < w; ++x) {
        int sum = 0;
        for (int y = 0; y < h; ++y)
            sum += (bin->data[y*w + x] < 128); // noir
        col[x] = (double)sum;
    }
    
    double mean = 0.0;
    for (int x = 0; x < w; ++x) mean += col[x];
    mean /= (double)w;
    double var = 0.0;
    for (int x = 0; x < w; ++x) {
        double d = col[x] - mean;
        var += d*d;
    }
    var /= (double)w;
    free(col);
    return var;
}

Image* auto_rotate(Image *src, float min_deg, float max_deg, float step_deg) {
    if (!src || !src->data || src->channels != 1) return NULL;

    // On travaille sur une image binaire (meilleure projection).
    // Si src est grise, binarise vite fait à 128 avant d’appeler cette fonction.
    Image *work = src;

    // Réduire pour accélérer la recherche d’angle
    Image *small = downscale_half(work);
    if (!small) return NULL;

    double best_score = -1.0;
    float best_angle = 0.0f;

    for (float a = min_deg; a <= max_deg + 1e-3f; a += step_deg) {
        Image *rot = rotate_image(small, a);
        if (!rot) continue;
        double score = vertical_projection_variance(rot);
        if (score > best_score) { best_score = score; best_angle = a; }
        free_image(rot);
    }
    free_image(small);

    // Appliquer l'angle optimal sur l'image d'origine
    return rotate_image(src, best_angle);
}
