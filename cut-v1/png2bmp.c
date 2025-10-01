#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: %s <input.png> <output.bmp>\n", argv[0]);
        return -1;
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Impossible d'initialiser SDL: %s\n", SDL_GetError());
        return -1;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        printf("Impossible d'initialiser SDL_image: %s\n", IMG_GetError());
        SDL_Quit();
        return -1;
    }

    SDL_Surface* image = IMG_Load(argv[1]);
    if (!image) {
        printf("Impossible de charger l'image PNG: %s\n", IMG_GetError());
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    if (SDL_SaveBMP(image, argv[2]) != 0) {
        printf("Impossible de sauvegarder le BMP: %s\n", SDL_GetError());
        SDL_FreeSurface(image);
        IMG_Quit();
        SDL_Quit();
        return -1;
    }

    printf("Conversion terminée : %s -> %s\n", argv[1], argv[2]);

    SDL_FreeSurface(image);
    IMG_Quit();
    SDL_Quit();
    return 0;
}

