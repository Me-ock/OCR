#include <stdio.h>
#include "image_loader.h"
#include "preprocessing.h"

int main() {
    Image *img = load_image("tests/test1.png");
    if (!img) return 1;

    Image *gray = to_grayscale(img);
    Image *norm = normalize_contrast(gray);              // bonus
    Image *bin  = to_binary(norm ? norm : gray, 128);

    // D: débruitage
    Image *den  = denoise_image_median3x3(bin);

    // C (déjà fait) : rotation manuelle possible ici si tu veux tester
    // float angle = 10.0f; Image *rot = rotate_image(den ? den : bin, angle);

    // Bonus : rotation auto
    Image *rotA = auto_rotate(den ? den : bin, -8.0f, 8.0f, 0.5f);

    save_image("processed_images/step_gray.png", gray);
    if (norm) save_image("processed_images/step_norm.png", norm);
    save_image("processed_images/step_bin.png",  bin);
    if (den)  save_image("processed_images/step_denoise.png", den);
    if (rotA) save_image("processed_images/step_autorot.png", rotA);

    free_image(img);
    free_image(gray);
    if (norm) free_image(norm);
    free_image(bin);
    if (den)  free_image(den);
    if (rotA) free_image(rotA);

    puts("Prétraitement terminé. Résultats dans processed_images/.");
    return 0;
}


