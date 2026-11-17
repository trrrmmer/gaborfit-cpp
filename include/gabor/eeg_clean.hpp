#pragma once

#include "gabor/gabor.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace gabor::eeg {

struct Options {
  double median_filter_duration_s = 0.1;
  double analysis_window_width_s = 2.0;
  double minimum_peak_distance_s = 0.1;
  double fitting_window_width_s = 0.2;
  double maximum_event_width_s = 0.03;
  std::int32_t pass_count = 1;
  bool retain_stage_signals = false;
};

enum class Rejection : std::int32_t {
  none = 0,
  invalid_problem = 1,
  waveform_too_wide = 2,
  negative_explained_variance = 3,
  eccentric_envelope = 4,
  numerical_failure = 5
};

struct Event {
  std::size_t pass_index = 0;
  std::size_t peak_index = 0;
  std::ptrdiff_t window_start = 0;
  std::ptrdiff_t window_end = 0;
  Parameters initial{}, lower{}, upper{}, parameters{};
  Status solver_status = Status::invalid_argument;
  Rejection rejection = Rejection::invalid_problem;
  bool accepted = false;
  double objective = 0.0;
  double explained_variance = 0.0;
  std::int32_t iterations = 0, evaluations = 0, attempts = 0;
  bool qc_triggered = false, rescue_accepted = false, bound_active = false;
};

struct PassDiagnostics {
  std::size_t pass_index = 0;
  std::vector<std::size_t> threshold_peaks;
  std::vector<std::size_t> duration_valid_peaks;
  std::vector<std::size_t> selected_peaks;
  std::vector<double> running_median;
  std::vector<double> high_pass;
  std::vector<Event> events;
};

struct Result {
  bool valid = false;
  std::string diagnostic;
  std::vector<double> cleaned;
  std::vector<PassDiagnostics> passes;
};

class Cleaner {
 public:
  explicit Cleaner(std::size_t reserve_samples = 0);
  Result clean(const double* signal, std::size_t sample_count,
               double sampling_frequency_hz, const Options& options = {});

 private:
  Fitter fitter_;
};

}  // namespace gabor::eeg
