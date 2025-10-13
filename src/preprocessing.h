#ifndef PREPROCESSING_H
#define PREPROCESSING_H

#include "image_loader.h"

Image* to_grayscale(Image *src);
Image* to_binary(Image *src, unsigned char threshold);
Image* rotate_image(Image *src, float angle_degrees);



#endif

