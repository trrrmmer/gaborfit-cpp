#include "gabor/gabor.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>

namespace {
constexpr double pi = 3.14159265358979323846;
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }

void synthetic_case(double fs, std::size_t n, bool nonuniform, double noise) {
  std::vector<double> times(n), signal(n), weights(n, 1.0);
  for (std::size_t i = 0; i < n; ++i) {
    times[i] = static_cast<double>(i) / fs;
    if (nonuniform && i > 0 && i + 1 < n) times[i] += 0.08 / fs * std::sin(0.7 * i);
  }
  std::sort(times.begin(), times.end());
  const double duration = times.back() - times.front();
  gabor::Parameters truth{{2.75, std::min(0.12 * fs, 85.0), 0.42,
                           std::max(3.0 / fs, duration / 8.0), duration * 0.51}};
  std::mt19937_64 generator(90210 + n); std::normal_distribution<double> normal(0.0, noise);
  for (std::size_t i = 0; i < n; ++i) signal[i] = gabor::Fitter::evaluate(times[i], truth) + normal(generator);
  if (n > 10) weights[3] = weights[n - 4] = 0.0;
  gabor::FitRequest request;
  request.samples = {signal.data(), nonuniform ? times.data() : nullptr, weights.data(), n, fs};
  request.bounds.lower = {{0.5, 1.0, -pi, 1.0 / fs, 0.2 * duration}};
  request.bounds.upper = {{5.0, 0.45 * fs, pi, 0.45 * duration, 0.8 * duration}};
  request.initial = {{2.4, truth[1] * 1.04, 0.25, truth[3] * 1.15, truth[4] * 0.999}};
  request.options.quality_control = true;
  gabor::Fitter fitter(n); const auto result = fitter.fit(request);
  if (!(result.objective < std::max(2e-6, 4.0 * noise * noise)))
    std::cerr << "case fs=" << fs << " n=" << n << " nonuniform=" << nonuniform
              << " objective=" << result.objective << " status=" << static_cast<int>(result.status)
              << " A=" << result.parameters[0] << " f=" << result.parameters[1]
              << " phase=" << result.parameters[2] << " sigma=" << result.parameters[3]
              << " centre=" << result.parameters[4] << " iterations=" << result.iterations << '\n';
  require(result.status == gabor::Status::converged, "synthetic fit did not converge");
  require(result.objective < std::max(2e-6, 4.0 * noise * noise), "synthetic objective too large");
  require(std::abs(result.parameters[1] - truth[1]) < std::max(0.5, truth[1] * 0.02), "frequency inaccurate");
  require(std::abs(result.parameters[3] - truth[3]) < std::max(2.0 / fs, truth[3] * 0.1), "sigma inaccurate");
  for (std::size_t p = 0; p < 5; ++p)
    require(result.parameters[p] >= request.bounds.lower[p] && result.parameters[p] <= request.bounds.upper[p],
            "parameter escaped bounds");
}
}

int main() {
  try {
    // Analytic model Jacobian versus parameter-scaled central differences.
    gabor::Parameters derivative_point{{2.3,83.0,.37,.013,.102}};
    const auto analytic=gabor::Fitter::jacobian(.094,derivative_point);
    for(std::size_t k=0;k<5;++k){
      auto left=derivative_point,right=derivative_point;
      double h=1e-6*std::max(1.0,std::abs(derivative_point[k]));if(k==3||k==4)h=1e-8;
      left[k]-=h;right[k]+=h;
      const double numeric=(gabor::Fitter::evaluate(.094,right)-gabor::Fitter::evaluate(.094,left))/(2*h);
      require(std::abs(numeric-analytic[k])<1e-5*std::max(1.0,std::abs(numeric)),"analytic Jacobian mismatch");
    }
    const double rates[] = {250, 500, 1000, 2000, 4000};
    const std::size_t lengths[] = {51, 101, 201, 401, 801};
    for (double rate : rates) for (std::size_t length : lengths)
      synthetic_case(rate, length, false, 1e-4);
    synthetic_case(1000, 201, true, 1e-4);
    synthetic_case(500, 101, false, 0.02);

    // Automatic initialization and negative signed-amplitude bounds.
    const std::size_t n = 201; const double fs = 1000;
    std::vector<double> signal(n); gabor::Parameters truth{{-3.0, 73.0, -0.3, 0.014, 0.101}};
    for (std::size_t i = 0; i < n; ++i) signal[i] = gabor::Fitter::evaluate(i / fs, truth);
    gabor::FitRequest request; request.samples = {signal.data(), nullptr, nullptr, n, fs};
    request.bounds.lower = {{-5.0, 10.0, -pi, 0.003, 0.07}};
    request.bounds.upper = {{-0.5, 200.0, pi, 0.04, 0.13}};
    request.options.automatic_initialization = true;
    const auto result = gabor::Fitter(n).fit(request);
    require(result.status == gabor::Status::converged, "automatic negative-amplitude fit failed");
    require(result.parameters[0] < 0.0, "negative amplitude convention violated");

    // Invalid inputs have stable diagnostic failure, never an exception.
    request.samples.sampling_frequency_hz = 0.0;
    require(gabor::Fitter(n).fit(request).status == gabor::Status::invalid_argument,
            "invalid sampling frequency accepted");
    std::cout << "All native Gabor tests passed; backend=" << gabor::Fitter::backend_name() << '\n';
    return 0;
  } catch (const std::exception& exception) {
    std::cerr << "TEST FAILURE: " << exception.what() << '\n'; return 1;
  }
}

