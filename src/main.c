#include <stdio.h>
#include "image_loader.h"
#include "preprocessing.h"

int main() {
    Image *img = load_image("tests/test1.png");
    if (!img) return 1;

    Image *gray = to_grayscale(img);
    if (!gray) return 1;

    Image *bin = to_binary(gray, 128);  // seuil = 128
    if (!bin) return 1;

    save_image("processed_images/output_binary.png", bin);

    free_image(img);
    free_image(gray);
    free_image(bin);

    printf("Image convertie en noir et blanc et sauvegardée.\n");
    return 0;
}

