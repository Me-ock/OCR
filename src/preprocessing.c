#include <stdlib.h>
#include "preprocessing.h"

Image* to_grayscale(Image *src) {
    if (!src || !src->data) return NULL;

    Image *gray = malloc(sizeof(Image));
    if (!gray) return NULL;

    gray->width = src->width;
    gray->height = src->height;
    gray->channels = 1; // une seule couche
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

