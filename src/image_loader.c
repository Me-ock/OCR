#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <stdio.h>
#include <stdlib.h>
#include "image_loader.h"

Image* load_image(const char *filename) {
    Image *img = malloc(sizeof(Image));
    if (!img) return NULL;

    img->data = stbi_load(filename, &img->width, &img->height, &img->channels, 0);
    if (!img->data) {
        free(img);
        fprintf(stderr, "Erreur: impossible de charger %s\n", filename);
        return NULL;
    }
    return img;
}

int save_image(const char *filename, Image *img) {
    if (!img || !img->data) return 0;
    return stbi_write_png(filename, img->width, img->height, img->channels, img->data, img->width * img->channels);
}
// les 2 sont assez evidents


void free_image(Image *img) {
    if (img) {
        stbi_image_free(img->data);
        free(img);
    }
}
// libere la memoire quand plus besoin de l'image 
