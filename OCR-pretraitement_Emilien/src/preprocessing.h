#ifndef PREPROCESSING_H
#define PREPROCESSING_H

#include "image_loader.h"

Image* enhance_contrast(Image *src);
Image* to_grayscale(Image *src);
void remove_grid_lines(Image *src);
Image* to_binary(Image *src, unsigned char threshold);
Image* to_binary_auto(Image* src);
Image* straighten_grid(Image *src);
Image* denoise_image_median3x3(Image *src);
Image* remove_small_components(Image *src, int min_size);

#endif

