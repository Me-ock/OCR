#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "image_loader.h"
#include "preprocessing.h"

#define TESTS_DIR   "tests"
#define OUTPUT_DIR  "processed_images"

static int has_image_extension(const char *name)
{
    const char *ext = strrchr(name, '.');
    if (!ext) return 0;

    return strcmp(ext, ".png")  == 0 || strcmp(ext, ".PNG")  == 0 ||
           strcmp(ext, ".jpg")  == 0 || strcmp(ext, ".JPG")  == 0 ||
           strcmp(ext, ".jpeg") == 0 || strcmp(ext, ".JPEG") == 0 ||
           strcmp(ext, ".bmp")  == 0 || strcmp(ext, ".BMP")  == 0;
}

static int is_dot_entry(const char *name)
{
    return (name[0] == '.' && (name[1] == '\0' ||
           (name[1] == '.' && name[2] == '\0')));
}

int main(void)
{
    DIR *dir = opendir(TESTS_DIR);
    if (!dir)
        return 1;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (is_dot_entry(entry->d_name))
            continue;
        if (!has_image_extension(entry->d_name))
            continue;

        char input_path[512];
        char output_path[512];
        snprintf(input_path, sizeof(input_path), "%s/%s", TESTS_DIR, entry->d_name);
        snprintf(output_path, sizeof(output_path), "%s/final_%s", OUTPUT_DIR, entry->d_name);

        Image *img = load_image(input_path);
        if (!img)
            continue;

        Image *gray = to_grayscale(img);
        free_image(img);
        if (!gray)
            continue;

        // straighten_grid peut soit retourner gray, soit une nouvelle image
        Image *deskew = straighten_grid(gray);
        if (!deskew) {
            free_image(gray);
            continue;
        }
        if (deskew != gray) {
            // nouvelle image -> on peut libérer l’ancienne
            free_image(gray);
        }
        // sinon deskew == gray, on ne la libère qu'une seule fois plus tard

        Image *bin = to_binary_auto(deskew);
        if (deskew == bin) {
            // au cas où to_binary_auto réutiliserait le même pointeur (peu probable, mais sûr)
            // on ne free deskew qu'une fois plus tard (via free_image(bin))
        } else {
            free_image(deskew);
        }
        if (!bin)
            continue;

        Image *clean = denoise_image_median3x3(bin);
        if (clean != bin) {
            free_image(bin);
        }
        if (!clean)
            continue;

        save_image(output_path, clean);
        free_image(clean);
    }

    closedir(dir);
    return 0;
}
