#include <stdio.h>
#include "data.h"

// Lettres simplifiées codées en 3x3 pixels
// 1 = pixel allumé, 0 = pixel éteint
float A[INPUT_SIZE] = {
    0, 1, 0,
    1, 0, 1,
    1, 1, 1
};

float B[INPUT_SIZE] = {
    1, 1, 0,
    1, 0, 1,
    1, 1, 0
};

// Labels cibles (supervision)
float label_A[OUTPUT_SIZE] = { 0.0f }; // A → 0
float label_B[OUTPUT_SIZE] = { 1.0f }; // B → 1

// Fonction d'affichage utile pour debug
void print_input(float *input) {
    for (int i = 0; i < 9; i++) {
        printf("%d ", (int)input[i]);
        if ((i + 1) % 3 == 0)
            printf("\n");
    }
}

