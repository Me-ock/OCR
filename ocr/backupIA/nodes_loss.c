#include <math.h>
double mse_nl(double output_activation, double expected_output) {
  double err = output_activation - expected_output;
  return err * err;
}
double mse_nl_d(double output_activation, double expected_output) {
  return 2 * (output_activation - expected_output);
}

double mse_w_constant_nl(double output_activation, double expected_output) {
  double err = output_activation - expected_output;
  return err * err * 0.2;
}

double abs_nl(double output_activation, double expected_output) {
  double err = output_activation - expected_output;
  return fabs(err);
}

