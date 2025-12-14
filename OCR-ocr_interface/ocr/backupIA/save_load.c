#include "defs.h"
#include "activation_functions.h"
#include "nodes_loss.h"
#include <stdio.h>
#include <stdlib.h>

enum activation_type get_activation_id(double (*fn)(double)) {
    if (fn == sigmoid_af) return ACTIVATION_SIGMOID;
    // if (fn == relu_af) return ACTIVATION_RELU; // TODO: Add ReLU support in enum
    return ACTIVATION_SIGMOID;
}

enum loss_type get_loss_id(double (*fn)(double, double)) {
    if (fn == mse_nl) return LOSS_MSE;
    // if (fn == cross_entropy_loss) return LOSS_CROSS_ENTROPY;
    return LOSS_MSE;
}

void set_activation_functions(struct neural_network *n, enum activation_type t) {
    (void)t;
    // Default to ReLU for hidden, Sigmoid for output (as per new architecture)
    // Ideally we should save per-layer activation, but for now we hardcode the new standard
    n->activation_function = relu_af;
    n->activation_function_d = relu_af_d;
    
    n->output_activation_function = sigmoid_af;
    n->output_activation_function_d = sigmoid_af_d;
}

void set_loss_functions(struct neural_network *n, enum loss_type t) {
    switch (t) {
        case LOSS_MSE:
            n->node_loss = mse_nl;
            n->node_loss_d = mse_nl_d;
            break;
    }
}

int save_network(const char *filename, struct neural_network *nn) {
    FILE *f = fopen(filename, "wb");
    if (!f) return -1;

    int activation_id = get_activation_id(nn->activation_function);
    int loss_id = get_loss_id(nn->node_loss);

    fwrite(&nn->number_of_layers, sizeof(int), 1, f);
    fwrite(&activation_id, sizeof(int), 1, f);
    fwrite(&loss_id, sizeof(int), 1, f);

    for (int l = 0; l < nn->number_of_layers; l++) {
        struct layer *ly = &nn->layers[l];

        fwrite(&ly->nodes_in, sizeof(int), 1, f);
        fwrite(&ly->nodes_out, sizeof(int), 1, f);

        // Store weights as [input][output]
        for (int i = 0; i < ly->nodes_in; i++) {
            fwrite(ly->weights[i], sizeof(double), ly->nodes_out, f);
        }

        fwrite(ly->biases, sizeof(double), ly->nodes_out, f);
    }

    fclose(f);
    return 0;
}

struct neural_network *load_network(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;

    struct neural_network *nn = malloc(sizeof(struct neural_network));
    if (!nn) { fclose(f); return NULL; }

    int activation_id, loss_id;
    fread(&nn->number_of_layers, sizeof(int), 1, f);
    fread(&activation_id, sizeof(int), 1, f);
    fread(&loss_id, sizeof(int), 1, f);

    set_activation_functions(nn, activation_id);
    set_loss_functions(nn, loss_id);

    nn->layers = malloc(sizeof(struct layer) * nn->number_of_layers);

    for (int l = 0; l < nn->number_of_layers; l++) {
        struct layer *ly = &nn->layers[l];
        ly->n = nn;

        fread(&ly->nodes_in, sizeof(int), 1, f);
        fread(&ly->nodes_out, sizeof(int), 1, f);

        // Allocate as [nodes_in][nodes_out] to match your forward pass
        ly->weights = malloc(sizeof(double*) * ly->nodes_in);
        for (int i = 0; i < ly->nodes_in; i++) {
            ly->weights[i] = malloc(sizeof(double) * ly->nodes_out);
            fread(ly->weights[i], sizeof(double), ly->nodes_out, f);
        }

        ly->biases = malloc(sizeof(double) * ly->nodes_out);
        fread(ly->biases, sizeof(double), ly->nodes_out, f);

        // Init grads/dumps/deltas
        ly->loss_gradient_weights = malloc(sizeof(double*) * ly->nodes_in);
        for (int i = 0; i < ly->nodes_in; i++) {
            ly->loss_gradient_weights[i] = calloc(ly->nodes_out, sizeof(double));
        }

        ly->loss_gradient_biases = calloc(ly->nodes_out, sizeof(double));
        ly->nodes_val_dump = calloc(ly->nodes_out, sizeof(double));
        ly->deltas = calloc(ly->nodes_out, sizeof(double));
    }

    fclose(f);
    return nn;
}

