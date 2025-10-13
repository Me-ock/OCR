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
