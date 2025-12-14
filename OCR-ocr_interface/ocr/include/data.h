#ifndef DATA_H
#define DATA_H

#define INPUT_SIZE 9   // 3x3 pixels pour l'instant
#define OUTPUT_SIZE 1  // Sortie binaire : 0 pour A, 1 pour B

extern float A[INPUT_SIZE];
extern float B[INPUT_SIZE];

extern float label_A[OUTPUT_SIZE];
extern float label_B[OUTPUT_SIZE];

void print_input(float *input);

#endif

