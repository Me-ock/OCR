#include <stdio.h>
#include "image_loader.h"
#include "preprocessing.h"

int main() {
    Image *img = load_image("tests/test1.png");
    if (!img) return 1;

    Image *gray = to_grayscale(img);
    if (!gray) return 1;

    save_image("processed_images/output.png", gray);

    free_image(img);
    free_image(gray);

    printf("Image convertie et sauvegardée dans processed_images/output.png\n");
    return 0;
}

