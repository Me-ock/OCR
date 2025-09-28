#ifndef IMAGE_LOADER_H
#define IMAGE_LOADER_H

typedef struct {
    int width;
    int height;
    int channels; // nb de canaux (ex : 3 = RGB)
    unsigned char *data;
} Image;

Image* load_image(const char *filename);
int save_image(const char *filename, Image *img);
void free_image(Image *img);

#endif

