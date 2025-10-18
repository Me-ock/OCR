#ifndef PREPROCESSING_H
#define PREPROCESSING_H

#include "image_loader.h"

Image* to_grayscale(Image *src);
Image* to_binary(Image *src, unsigned char threshold);
Image* rotate_image(Image *src, float angle_degrees);
Image* denoise_image_median3x3(Image *src);          
Image* normalize_contrast(Image *src);               
Image* auto_rotate(Image *src, float min_deg, float max_deg, float step_deg); 

#endif

