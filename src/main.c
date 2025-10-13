#include <stdio.h>
#include "image_loader.h"
#include "preprocessing.h"

int main() {
    Image *img = load_image("tests/test1.png");
    if (!img) return 1;

    Image *gray = to_grayscale(img);
    Image *bin = to_binary(gray, 128);

    float angle;
    printf("Entrez un angle de rotation (ex: 15 ou -10) : ");
    scanf("%f", &angle);

    Image *rotated = rotate_image(bin, angle);
    save_image("processed_images/output_rotated.png", rotated);

    free_image(img);
    free_image(gray);
    free_image(bin);
    free_image(rotated);

    printf("Image tournée sauvegardée sous processed_images/output_rotated.png\n");
    return 0;
}

