#include "gabor/gabor.hpp"
#include "gabor/gabor_c.h"
#include "gabor/eeg_clean.hpp"
#include <cmath>
#include <iostream>
#include <vector>

int main() {
  const gabor::Parameters parameters{{2.0, 50.0, 0.25, 0.012, 0.1}};
  const double cxx = gabor::Fitter::evaluate(0.1, parameters);
  const double c_parameters[5] = {2.0, 50.0, 0.25, 0.012, 0.1};
  const double c = gaborfit_evaluate(0.1, c_parameters);
  if (gaborfit_abi_version() != GABORFIT_ABI_VERSION || std::abs(cxx - c) > 1e-15) return 1;
  std::vector<double> signal(500,0.0);gabor::eeg::Cleaner cleaner(signal.size());
  const auto cleaned=cleaner.clean(signal.data(),signal.size(),500.0);
  if(!cleaned.valid||cleaned.cleaned.size()!=signal.size())return 1;
  std::cout << "Installed fitter and EEG cleaner package consumer passed\n";
  return 0;
}
