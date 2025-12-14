#include "defs.h"
#include "neural_network.h"

void dump_network(struct neural_network *n, double *inputs) {
  double *res = inputs;
  for(int i = 0; i < n->number_of_layers; i++) {
    n->layers[i].nodes_val_dump = calculate_layer_outputs(&n->layers[i], res);
    res = n->layers[i].nodes_val_dump;
  }
}

