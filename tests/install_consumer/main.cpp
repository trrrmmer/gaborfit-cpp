#include "gabor/gabor.hpp"
#include "gabor/gabor_c.h"
#include <cmath>
#include <iostream>

int main() {
  const gabor::Parameters parameters{{2.0, 50.0, 0.25, 0.012, 0.1}};
  const double cxx = gabor::Fitter::evaluate(0.1, parameters);
  const double c_parameters[5] = {2.0, 50.0, 0.25, 0.012, 0.1};
  const double c = gaborfit_evaluate(0.1, c_parameters);
  if (gaborfit_abi_version() != GABORFIT_ABI_VERSION || std::abs(cxx - c) > 1e-15) return 1;
  std::cout << "Installed C and C++ package consumer passed\n";
  return 0;
}
