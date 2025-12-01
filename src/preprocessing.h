#ifndef PREPROCESSING_H
#define PREPROCESSING_H

#include "image_loader.h"

Image* to_grayscale(Image *src);
Image* to_binary(Image *src, unsigned char threshold);
Image* to_binary_auto(Image* src);
Image* straighten_grid(Image *src);

#endif

