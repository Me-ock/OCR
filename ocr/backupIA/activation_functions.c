#include <math.h>


double simple_af(double weighted_input) {
  return weighted_input;
}
double step_af(double weighted_input) {
  return (weighted_input > 0) ? 1 : 0;
}
double sigmoid_af(double weighted_input) {
  return 1 / (1 + exp(-weighted_input));
}
double sigmoid_af_d(double weighted_input) {
  // assumes weighted_input = sigmoid(z)
  return weighted_input * (1. - weighted_input);
}

double relu_af(double weighted_input) {
  return (weighted_input > 0) ? weighted_input : 0;
}

double relu_af_d(double weighted_input) {
  return (weighted_input > 0) ? 1 : 0;
}
