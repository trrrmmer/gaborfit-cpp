#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#if defined(_WIN32)
# if defined(GABORFIT_BUILDING_LIBRARY)
#  define GABORFIT_CPP_API __declspec(dllexport)
# else
#  define GABORFIT_CPP_API __declspec(dllimport)
# endif
#else
# define GABORFIT_CPP_API __attribute__((visibility("default")))
#endif

namespace gabor {

constexpr std::size_t parameter_count = 5;
using Parameters = std::array<double, parameter_count>;

enum class Status : std::int32_t {
  converged = 0, valid_nonconverged = 1, invalid_argument = -1,
  numerical_failure = -2, internal_error = -3
};

struct Samples {
  const double* signal = nullptr;
  const double* times = nullptr;       // Optional; seconds.
  const double* weights = nullptr;     // Optional nonnegative weights; zero masks a sample.
  std::size_t size = 0;
  double sampling_frequency_hz = 0.0;  // Used only when times == nullptr.
};

struct Bounds { Parameters lower{}; Parameters upper{}; };

struct SolverOptions {
  std::int32_t max_iterations = 100;
  std::int32_t fallback_iterations = 200;
  double x_tolerance = 1e-5;
  double f_tolerance = 1e-5;
  double gradient_tolerance = 1e-8;
  double initial_damping = 1e-3;
  bool automatic_initialization = false;
  bool deterministic_fallback = true;
  bool quality_control = true;
  double qc_unexplained_energy = 0.4;
  double qc_required_relative_improvement = 1e-3;
};

struct FitRequest {
  Samples samples;
  Parameters initial{}; // [amplitude, frequency Hz, phase rad, sigma s, centre s]
  Bounds bounds{};
  SolverOptions options{};
};

struct FitResult {
  Status status = Status::invalid_argument;
  Parameters parameters{};
  double objective = 0.0;
  double residual_rms = 0.0;
  std::int32_t iterations = 0;
  std::int32_t evaluations = 0;
  std::int32_t attempts = 0;
  bool qc_triggered = false;
  bool rescue_accepted = false;
  bool bound_active = false;
  std::string diagnostic;
};

class GABORFIT_CPP_API Fitter {
 public:
  explicit Fitter(std::size_t reserve_samples = 0);
  FitResult fit(const FitRequest& request);
  static double evaluate(double time_seconds, const Parameters& parameters) noexcept;
  static Parameters jacobian(double time_seconds, const Parameters& parameters) noexcept;
  static const char* backend_name() noexcept;

 private:
  std::vector<double> times_, weights_, residual_, trial_residual_;
  std::vector<double> jacobian_;
};

GABORFIT_CPP_API Parameters automatic_initialization(const Samples& samples, const Bounds& bounds);
GABORFIT_CPP_API bool validate_request(const FitRequest& request, std::string& error);

} // namespace gabor

