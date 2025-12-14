#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "defs.h"

struct neural_network *create_neural_network(int *layers_size, int size,
    double (*activation_function)(double),
    double (*output_activation_function)(double),
    double (*node_loss)(double, double),
    double (*activation_function_d)(double),
    double (*output_activation_function_d)(double),
    double (*node_loss_d)(double, double)) {


  struct neural_network *res = malloc(sizeof(struct neural_network));
  struct layer *layers = malloc(sizeof(struct layer) * (size - 1));

  res->activation_function = activation_function;
  res->output_activation_function = output_activation_function;
  res->node_loss = node_loss;
  res->activation_function_d = activation_function_d;
  res->output_activation_function_d = output_activation_function_d;
  res->node_loss_d = node_loss_d;

  res->layers = layers;
  res->number_of_layers = size - 1;

  for (int i = 0; i < size - 1; i++) {
    int nodes_in = layers_size[i];
    int nodes_out = layers_size[i+1];

    double *biases = malloc(sizeof(double) * nodes_out);
    double **weights = malloc(sizeof(double*) * nodes_in);
    for (int j = 0; j < nodes_in; j++) {
      weights[j] = malloc(sizeof(double) * nodes_out);
    }

    double *gradient_biases = malloc(sizeof(double) * nodes_out);
    double **gradient_weights = malloc(sizeof(double*) * nodes_in);
    for (int j = 0; j < nodes_in; j++) {
      gradient_weights[j] = malloc(sizeof(double) * nodes_out);
    }

    double *deltas = malloc(sizeof(double) * nodes_out);

    res->layers[i] = (struct layer){
      .nodes_in = nodes_in,
        .n = res,
        .nodes_out = nodes_out,
        .biases = biases,
        .weights = weights,

        .loss_gradient_weights = gradient_weights,
        .loss_gradient_biases = gradient_biases,
        .nodes_val_dump = NULL,
        .deltas = deltas,
    };
  }

  return res;
}

void fill_random_layer(struct layer *l) {
  // He Initialization (better for ReLU)
  // Uniform distribution [-limit, limit] where limit = sqrt(6 / nodes_in)
  double limit = sqrt(6.0 / l->nodes_in);
  
  for(int i = 0; i < l->nodes_out; i++) {
    l->biases[i] = 0;
  }
  for(int i = 0; i < l->nodes_in; i++) {
    for(int j = 0; j < l->nodes_out; j++) {
      l->weights[i][j] = ((double)rand() / RAND_MAX) * 2 * limit - limit;
    }
  }
}

void fill_random_neural_network(struct neural_network *n) {
  for(int i = 0; i < n->number_of_layers; i++) {
    fill_random_layer(&n->layers[i]);
  }
}

void print_layer_with_dump(struct layer *l) {
  printf(" dump : \n   ");
  for(int i = 0; i < l->nodes_out; i++) {
    printf("%f ", l->nodes_val_dump[i]);
  }
  printf("\n biases : \n   ");
  for(int i = 0; i < l->nodes_out; i++) {
    printf("%f ", l->biases[i]);
  }
  printf("\n weights : \n");
  for(int i = 0; i < l->nodes_in; i++) {
    printf("  from %d:\n    ", i);
    for(int j = 0; j < l->nodes_out; j++) {
      printf("%f ", l->weights[i][j]);
    }
    printf("\n");
  }
  printf("\n");
}

void print_neural_network_with_dump(struct neural_network *n) {
  printf("---- Neural Network S ----\n\n");
  printf("Number of inputs : %d\n\n", n->layers[0].nodes_in);
  for(int i = 0; i < n->number_of_layers; i++) {
    printf("Layer %d (%d):\n", i, n->layers[i].nodes_out);
    print_layer_with_dump(&n->layers[i]);
  }
  printf("---- Neural Network E ----\n");
}

void print_layer(struct layer *l) {
  printf(" biases : \n   ");
  for(int i = 0; i < l->nodes_out; i++) {
    printf("%f ", l->biases[i]);
  }
  printf("\n weights : \n");
  for(int i = 0; i < l->nodes_in; i++) {
    printf("  from %d:\n    ", i);
    for(int j = 0; j < l->nodes_out; j++) {
      printf("%f ", l->weights[i][j]);
    }
    printf("\n");
  }
  printf("\n");
}

void print_neural_network(struct neural_network *n) {
  printf("---- Neural Network S ----\n\n");
  printf("Number of inputs : %d\n\n", n->layers[0].nodes_in);
  for(int i = 0; i < n->number_of_layers; i++) {
    printf("Layer %d (%d):\n", i, n->layers[i].nodes_out);
    print_layer(&n->layers[i]);
  }
  printf("---- Neural Network E ----\n");
}

double *calculate_layer_outputs(struct layer *l, double *inputs) {
  double *activations = malloc(sizeof(double) * l->nodes_out);

  int is_output = (l == &l->n->layers[l->n->number_of_layers - 1]);

  for (int i = 0; i < l->nodes_out; i++) {
    double weighted_input = l->biases[i];
    for (int j = 0; j < l->nodes_in; j++) {
      // Correction: l->weights est [nodes_in][nodes_out]
      // Donc weights[j][i] est correct si j < nodes_in et i < nodes_out
      // inputs doit être de taille nodes_in
      weighted_input += inputs[j] * l->weights[j][i];
    }
    
    if (is_output)
        activations[i] = l->n->output_activation_function(weighted_input);
    else
        activations[i] = l->n->activation_function(weighted_input);

#ifdef DEBUG_LAYER_ACTIVATION
    printf("%f ", activations[i]);
#endif
  }
#ifdef DEBUG_LAYER_ACTIVATION
  printf("\n");
#endif

  return activations;
}

double *calculate_neural_network_outputs(struct neural_network *n, double *inputs) {

  // IMPORTANT: On ne doit PAS modifier 'inputs' car il appartient à l'appelant.
  // La première couche prend 'inputs' mais ne doit pas le free.
  // Les couches suivantes prennent le résultat de la précédente (qui est malloc) et doivent le free.
  
  double *current_input = inputs;
  double *to_free = NULL; // Pointeur vers la mémoire à libérer (résultat de la couche précédente)

#ifdef DEBUG_LAYER_ACTIVATION
  printf("\n");
#endif

  for(int i = 0; i < n->number_of_layers; i++) {

#ifdef DEBUG_LAYER_ACTIVATION
    if (i == n->number_of_layers-1) {
      printf("layer out : ");
    } else {
      printf("layer %d : ", i);
    }
#endif

    double *layer_output = calculate_layer_outputs(&n->layers[i], current_input);
    
    // Si on avait alloué une mémoire temporaire pour l'input précédent, on la libère
    if (to_free != NULL) {
      free(to_free);
      to_free = NULL; // Sécurité
    }

    // Le nouvel input pour la prochaine couche est l'output de celle-ci
    current_input = layer_output;
    // Et c'est ce pointeur qu'il faudra libérer au prochain tour
    to_free = layer_output;
    
    // VERIFICATION DE SECURITE POUR LA PROCHAINE COUCHE
    if (i + 1 < n->number_of_layers) {
        if (n->layers[i+1].nodes_in != n->layers[i].nodes_out) {
            printf("CRITICAL ERROR: Layer mismatch! Layer %d output size (%d) != Layer %d input size (%d)\n", 
                i, n->layers[i].nodes_out, i+1, n->layers[i+1].nodes_in);
            // On ne peut pas continuer, ça va crash
            if (to_free) free(to_free);
            return NULL; // Ou exit(1)
        }
    }
  }
  
  return current_input;
}

// get prediction of the nn
int classify(struct neural_network *n, double *inputs) {
  double *res = calculate_neural_network_outputs(n, inputs);
  int max = 0;
  for (int i = 1; i < n->layers[n->number_of_layers-1].nodes_out; i++) {
    if (res[max] < res[i]) {
      max = i;
    }
  }
  free(res);
  return max;
}

void drop_neural_network() {
  // drop... flemme
  // TODO
}

