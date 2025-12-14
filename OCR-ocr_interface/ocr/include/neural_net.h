#ifndef NEURAL_NET_H
#define NEURAL_NET_H
#define ALPHABET_SIZE 26
#include <stdlib.h>
#include <time.h>

typedef struct NeuralNetwork {
    int input_size;
    int hidden_size;
    int output_size;
    float *hidden_weights; // input → hidden
    float *output_weights; // hidden → output
    float *hidden_bias;
    float *output_bias;
    float *output;
} NeuralNetwork;

// Fonctions
NeuralNetwork *init_network(int input_size, int hidden_size, int output_size);
void free_network(NeuralNetwork *nn);
float sigmoid(float x);
float sigmoid_derivative(float x);
void forward_pass(NeuralNetwork *nn, float *inputs);
int validate(NeuralNetwork *nn, float **test_inputs, float **test_targets, int sample_count);
void train_epoch(NeuralNetwork *nn, float **train_inputs, float **train_targets, int sample_count, float lr);
void save_network(NeuralNetwork *nn, const char *filename);
NeuralNetwork *load_network(const char *filename);
char predict_character(NeuralNetwork *nn, float *inputs);
#endif
