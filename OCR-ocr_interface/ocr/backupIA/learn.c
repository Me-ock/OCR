#include "defs.h"
#include "utils.h"
#include "neural_network.h"


// also called "cost" in some literature
double get_point_loss(struct neural_network *n, double *inputs, double *expected) {
  double *outputs = calculate_neural_network_outputs(n,inputs);
  double loss = 0;

  for (int i = 0; i < n->layers[n->number_of_layers-1].nodes_out; i++) {
    loss += n->node_loss(outputs[i], expected[i]);
  }
  return loss;
}

double get_dataset_loss(struct neural_network *n, double **inputs, double **expected, int dataset_size) {
  double loss = 0;
  for (int i = 0; i < dataset_size; i++) {
    loss += get_point_loss(n,inputs[i], expected[i]);
  }
  return loss / dataset_size;
}

void apply_gradients_to_layer(struct layer *l, double learn_rate) {
  for(int b = 0; b < l->nodes_out; b++) {
    l->biases[b] -= l->loss_gradient_biases[b] * learn_rate;
  }

  for(int wi = 0; wi < l->nodes_in; wi++) {
    for(int wj = 0; wj < l->nodes_out; wj++) {
      l->weights[wi][wj] -= l->loss_gradient_weights[wi][wj] * learn_rate;
    }
  }
}

void apply_gradients_to_neural_network(struct neural_network *n, double learn_rate) {
  for(int i = 0; i < n->number_of_layers; i++) {
    apply_gradients_to_layer(&n->layers[i], learn_rate);
  }
}


void learn_one_epoch_using_discrete_method(struct neural_network *n, double **inputs, double **expected, int dataset_size, double learn_rate) {
  const double h = 0.00001;
  double og_loss = get_dataset_loss(n, inputs, expected, dataset_size);

  for(int i = 0; i < n->number_of_layers; i++) {
    struct layer *l = &n->layers[i];

    for(int b = 0; b < l->nodes_out; b++) {
      l->biases[b] += h;
      double delta_loss = get_dataset_loss(n, inputs, expected, dataset_size) - og_loss;
      l->biases[b] -= h;
      l->loss_gradient_biases[b] = delta_loss / h;
    }

    for(int wi = 0; wi < l->nodes_in; wi++) {
      for(int wj = 0; wj < l->nodes_out; wj++) {
        l->weights[wi][wj] += h;
        double delta_loss = get_dataset_loss(n, inputs, expected, dataset_size) - og_loss;
        l->weights[wi][wj] -= h;
        l->loss_gradient_weights[wi][wj] = delta_loss / h;
      }
    }
  }
  apply_gradients_to_neural_network(n, learn_rate);
}

void learn_unit_with_backpropagation(struct neural_network *n, double *inputs, double *expected, double learn_rate) {
  int L = n->number_of_layers;

  dump_network(n,inputs);


  // output layer delta
  struct layer *out = &n->layers[L - 1];

  for (int j = 0; j < out->nodes_out; j++) {
    double a = out->nodes_val_dump[j];
    out->deltas[j] = n->node_loss_d(a, expected[j]) * n->output_activation_function_d(a);
  }

  // hidden layers deltas (backwards)
  for (int l = L - 2; l >= 0; l--) {
    struct layer *layer = &n->layers[l];
    struct layer *next = &n->layers[l + 1];

    for (int i = 0; i < layer->nodes_out; i++) {
      double sum = 0.;
      for (int j = 0; j < next->nodes_out; j++) {
        sum += next->weights[i][j] * next->deltas[j];
      }
      layer->deltas[i] = sum * n->activation_function_d(layer->nodes_val_dump[i]);
    }
  }

  // update weights & biases
  double *prev = inputs;
  for (int l = 0; l < L; l++) {
    struct layer *layer = &n->layers[l];

    // input activations (previous layer or original input)
    if (l > 0) prev = n->layers[l - 1].nodes_val_dump;

    for (int j = 0; j < layer->nodes_out; j++) {
      for (int i = 0; i < layer->nodes_in; i++) {
        layer->weights[i][j] -= learn_rate * prev[i] * layer->deltas[j];
      }
      layer->biases[j] -= learn_rate * layer->deltas[j];
    }
  }
}

// do the basically same as learn_one_epoch_using_discrete_method but *way* less
// understandable so its way more optimized (computers are dicks sometimes)
void learn_epoch_with_backpropagation(struct neural_network *n, double **inputs, double **expected, int dataset_size, double learn_rate) {
  for(int i = 0; i < dataset_size; i++) {
    learn_unit_with_backpropagation(n, inputs[i], expected[i],learn_rate);
  }
}

