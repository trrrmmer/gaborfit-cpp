#include "gabor/gabor.hpp"

#include <Eigen/Dense>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <sstream>

namespace gabor {
namespace {
constexpr double pi = 3.141592653589793238462643383279502884;

double clamp_value(double value, double lower, double upper) {
  return std::max(lower, std::min(upper, value));
}

Parameters projected(Parameters value, const Bounds& bounds) {
  for (std::size_t i = 0; i < parameter_count; ++i)
    value[i] = clamp_value(value[i], bounds.lower[i], bounds.upper[i]);
  return value;
}

struct Prepared {
  std::size_t n = 0;
  double weight_sum = 0.0;
  bool uniform = false;
  double dt = 0.0;
};

Prepared prepare_samples(const Samples& input, std::vector<double>& times,
                         std::vector<double>& weights) {
  times.resize(input.size); weights.resize(input.size);
  if (input.times) std::copy(input.times, input.times + input.size, times.begin());
  else for (std::size_t i = 0; i < input.size; ++i)
    times[i] = static_cast<double>(i) / input.sampling_frequency_hz;
  if (input.weights) std::copy(input.weights, input.weights + input.size, weights.begin());
  else std::fill(weights.begin(), weights.end(), 1.0);
  Prepared out; out.n = input.size;
  out.weight_sum = std::accumulate(weights.begin(), weights.end(), 0.0);
  out.dt = input.size > 1 ? times[1] - times[0] : 0.0;
  out.uniform = out.dt > 0.0;
  const double tolerance = std::max(1e-12, std::abs(out.dt) * 1e-8);
  for (std::size_t i = 2; i < input.size && out.uniform; ++i)
    out.uniform = std::abs((times[i] - times[i - 1]) - out.dt) <= tolerance;
  return out;
}

double objective_and_jacobian(const Samples& samples, const std::vector<double>& times,
                              const std::vector<double>& weights, const Parameters& p,
                              std::vector<double>& residual, std::vector<double>* jacobian,
                              const std::vector<int>& active, double weight_sum) {
  const std::size_t n = samples.size; residual.resize(n);
  if (jacobian) jacobian->resize(n * active.size());
  double sum = 0.0;
  for (std::size_t i = 0; i < n; ++i) {
    const double dx = times[i] - p[4];
    const double envelope = std::exp(-0.5 * (dx / p[3]) * (dx / p[3]));
    const double angle = 2.0 * pi * p[1] * dx + p[2];
    const double sine = std::sin(angle), cosine = std::cos(angle);
    const double model = p[0] * envelope * cosine;
    const double root_weight = std::sqrt(weights[i]);
    const double r = root_weight * (model - samples.signal[i]);
    residual[i] = r; sum += r * r;
    if (jacobian) {
      const std::array<double, 5> full{{
        envelope * cosine,
        -p[0] * 2.0 * pi * dx * envelope * sine,
        -p[0] * envelope * sine,
        model * dx * dx / (p[3] * p[3] * p[3]),
        model * dx / (p[3] * p[3]) + p[0] * 2.0 * pi * p[1] * envelope * sine
      }};
      for (std::size_t c = 0; c < active.size(); ++c)
        (*jacobian)[i * active.size() + c] = root_weight * full[active[c]];
    }
  }
  return sum / weight_sum;
}

struct StageResult {
  Parameters parameters{};
  double objective = std::numeric_limits<double>::infinity();
  int iterations = 0, evaluations = 0;
  bool converged = false, numerical_failure = false, bound_active = false;
};

StageResult solve_stage(const Samples& samples, const Prepared& prepared,
                        const std::vector<double>& times, const std::vector<double>& weights,
                        const Bounds& bounds, Parameters start, const std::vector<int>& active,
                        const SolverOptions& options, int iteration_limit,
                        std::vector<double>& residual, std::vector<double>& trial_residual,
                        std::vector<double>& jacobian) {
  StageResult output; start = projected(start, bounds); output.parameters = start;
  const int m = static_cast<int>(active.size()); const int n = static_cast<int>(samples.size);
  output.objective = objective_and_jacobian(samples, times, weights, start, residual,
                                             &jacobian, active, prepared.weight_sum);
  ++output.evaluations;
  Eigen::VectorXd scale(m), q(m), qlower(m), qupper(m);
  for (int c = 0; c < m; ++c) {
    double norm2 = 0.0;
    for (int i = 0; i < n; ++i) {
      const double value = jacobian[static_cast<std::size_t>(i) * m + c]; norm2 += value * value;
    }
    const int index = active[c];
    scale[c] = norm2 > std::sqrt(std::numeric_limits<double>::epsilon())
      ? 1.0 / std::sqrt(norm2) : std::max((bounds.upper[index] - bounds.lower[index]) / 2.0,
                                          std::numeric_limits<double>::epsilon());
    q[c] = 0.0;
    qlower[c] = (bounds.lower[index] - start[index]) / scale[c];
    qupper[c] = (bounds.upper[index] - start[index]) / scale[c];
  }
  const Parameters origin = start;
  auto physical = [&](const Eigen::VectorXd& value) {
    Parameters p = output.parameters;
    for (int c = 0; c < m; ++c) p[active[c]] = origin[active[c]] + scale[c] * value[c];
    return projected(p, bounds);
  };
  double damping = std::max(options.initial_damping, 1e-15);
  Eigen::MatrixXd augmented(n + m, m); Eigen::VectorXd rhs(n + m), gradient(m), delta(m);
  for (int iteration = 0; iteration < iteration_limit; ++iteration) {
    output.iterations = iteration + 1;
    objective_and_jacobian(samples, times, weights, output.parameters, residual,
                           &jacobian, active, prepared.weight_sum);
    for (int i = 0; i < n; ++i)
      for (int c = 0; c < m; ++c)
        augmented(i, c) = jacobian[static_cast<std::size_t>(i) * m + c] * scale[c];
    Eigen::Map<Eigen::VectorXd> residual_map(residual.data(), n);
    gradient.noalias() = augmented.topRows(n).transpose() * residual_map;
    if (!gradient.allFinite()) { output.numerical_failure = true; break; }
    if (gradient.lpNorm<Eigen::Infinity>() / prepared.weight_sum <= options.gradient_tolerance) {
      output.converged = true; break;
    }
    augmented.bottomRows(m).setZero();
    for (int c = 0; c < m; ++c) augmented(n + c, c) = std::sqrt(damping);
    rhs.head(n) = -residual_map; rhs.tail(m).setZero();
    delta = augmented.colPivHouseholderQr().solve(rhs);
    if (!delta.allFinite()) { output.numerical_failure = true; break; }
    Eigen::VectorXd trial_q = q + delta;
    for (int c = 0; c < m; ++c) trial_q[c] = clamp_value(trial_q[c], qlower[c], qupper[c]);
    const Parameters trial = physical(trial_q);
    const double trial_objective = objective_and_jacobian(samples, times, weights, trial,
      trial_residual, nullptr, active, prepared.weight_sum); ++output.evaluations;
    if (std::isfinite(trial_objective) && trial_objective < output.objective) {
      const double previous_objective = output.objective;
      const double improvement = previous_objective - trial_objective;
      const double step = (trial_q - q).lpNorm<Eigen::Infinity>();
      q = trial_q; output.parameters = trial; output.objective = trial_objective;
      residual.swap(trial_residual); damping = std::max(damping / 3.0, 1e-15);
      if (step <= options.x_tolerance * (1.0 + q.lpNorm<Eigen::Infinity>()) ||
          improvement <= options.f_tolerance * std::max(1e-12, previous_objective)) {
        output.converged = true; break;
      }
    } else {
      damping = std::min(damping * 10.0, 1e15);
      if (damping >= 1e15) { output.numerical_failure = true; break; }
    }
  }
  for (std::size_t i = 0; i < parameter_count; ++i) {
    const double tolerance = 1e-10 * std::max(1.0, std::abs(bounds.upper[i] - bounds.lower[i]));
    output.bound_active = output.bound_active ||
      std::abs(output.parameters[i] - bounds.lower[i]) <= tolerance ||
      std::abs(output.parameters[i] - bounds.upper[i]) <= tolerance;
  }
  return output;
}

StageResult two_stage(const FitRequest& request, const Prepared& prepared,
                      const std::vector<double>& times, const std::vector<double>& weights,
                      Parameters start, int limit, std::vector<double>& residual,
                      std::vector<double>& trial, std::vector<double>& jacobian) {
  const std::vector<int> first{0, 1, 2}, second{0, 1, 2, 3, 4};
  StageResult a = solve_stage(request.samples, prepared, times, weights, request.bounds,
    start, first, request.options, limit, residual, trial, jacobian);
  StageResult b = solve_stage(request.samples, prepared, times, weights, request.bounds,
    a.parameters, second, request.options, limit, residual, trial, jacobian);
  b.iterations += a.iterations; b.evaluations += a.evaluations;
  b.converged = a.converged && b.converged; b.numerical_failure |= a.numerical_failure;
  b.bound_active |= a.bound_active; return b;
}

double signal_energy(const Samples& samples, const std::vector<double>& weights, double weight_sum) {
  double sum = 0.0; for (std::size_t i = 0; i < samples.size; ++i)
    sum += weights[i] * samples.signal[i] * samples.signal[i];
  return sum / weight_sum;
}
} // namespace

Fitter::Fitter(std::size_t reserve_samples) {
  times_.reserve(reserve_samples); weights_.reserve(reserve_samples);
  residual_.reserve(reserve_samples); trial_residual_.reserve(reserve_samples);
  jacobian_.reserve(reserve_samples * parameter_count);
}

double Fitter::evaluate(double t, const Parameters& p) noexcept {
  const double dx = t - p[4];
  return p[0] * std::exp(-0.5 * (dx / p[3]) * (dx / p[3])) *
    std::cos(2.0 * pi * p[1] * dx + p[2]);
}

Parameters Fitter::jacobian(double t, const Parameters& p) noexcept {
  const double dx=t-p[4], envelope=std::exp(-0.5*(dx/p[3])*(dx/p[3]));
  const double angle=2.0*pi*p[1]*dx+p[2], sine=std::sin(angle), cosine=std::cos(angle);
  const double model=p[0]*envelope*cosine;
  return {{envelope*cosine,-p[0]*2.0*pi*dx*envelope*sine,-p[0]*envelope*sine,
    model*dx*dx/(p[3]*p[3]*p[3]),model*dx/(p[3]*p[3])+p[0]*2.0*pi*p[1]*envelope*sine}};
}

const char* Fitter::backend_name() noexcept { return "eigen-bounded-lm-qr"; }

bool validate_request(const FitRequest& request, std::string& error) {
  const Samples& s = request.samples;
  if (!s.signal || s.size < 5) { error = "at least five signal samples are required"; return false; }
  if (!s.times && !(std::isfinite(s.sampling_frequency_hz) && s.sampling_frequency_hz > 0.0)) {
    error = "sampling_frequency_hz must be positive when times is null"; return false;
  }
  std::size_t positive_weights = 0;
  for (std::size_t i = 0; i < s.size; ++i) {
    if (!std::isfinite(s.signal[i])) { error = "signal contains nonfinite values"; return false; }
    if (s.times && (!std::isfinite(s.times[i]) || (i && s.times[i] <= s.times[i - 1]))) {
      error = "times must be finite and strictly increasing"; return false;
    }
    const double weight = s.weights ? s.weights[i] : 1.0;
    if (!std::isfinite(weight) || weight < 0.0) { error = "weights must be finite and nonnegative"; return false; }
    positive_weights += weight > 0.0;
  }
  if (positive_weights < 5) { error = "at least five samples must have positive weight"; return false; }
  for (std::size_t i = 0; i < parameter_count; ++i) {
    if (!std::isfinite(request.bounds.lower[i]) || !std::isfinite(request.bounds.upper[i]) ||
        request.bounds.lower[i] >= request.bounds.upper[i]) {
      error = "all parameter bounds must be finite and strictly ordered"; return false;
    }
    if (!request.options.automatic_initialization && !std::isfinite(request.initial[i])) {
      error = "initial parameters must be finite"; return false;
    }
  }
  if (request.bounds.lower[1] < 0.0 || request.bounds.lower[3] <= 0.0) {
    error = "frequency bounds must be nonnegative and sigma bounds positive"; return false;
  }
  if (request.options.max_iterations <= 0 || request.options.fallback_iterations <= 0) {
    error = "iteration limits must be positive"; return false;
  }
  return true;
}

Parameters automatic_initialization(const Samples& samples, const Bounds& bounds) {
  std::vector<double> times(samples.size), weights(samples.size, 1.0);
  if (samples.times) std::copy(samples.times, samples.times + samples.size, times.begin());
  else for (std::size_t i = 0; i < samples.size; ++i) times[i] = i / samples.sampling_frequency_hz;
  if (samples.weights) std::copy(samples.weights, samples.weights + samples.size, weights.begin());
  double absolute_sum = 0.0, centre_sum = 0.0; std::size_t peak = 0;
  for (std::size_t i = 0; i < samples.size; ++i) {
    const double value = weights[i] * std::abs(samples.signal[i]);
    absolute_sum += value; centre_sum += value * times[i];
    if (std::abs(samples.signal[i]) > std::abs(samples.signal[peak])) peak = i;
  }
  const double centre = absolute_sum > 0.0 ? centre_sum / absolute_sum : times[samples.size / 2];
  const double duration = times.back() - times.front();
  const double dt = duration / static_cast<double>(samples.size - 1);
  const double nyquist = 0.5 / dt;
  const double low = std::max(bounds.lower[1], 1.0 / std::max(duration, dt));
  const double high = std::min(bounds.upper[1], nyquist);
  double best_frequency = low, best_power = -1.0;
  const int bins = 256;
  for (int bin = 0; bin < bins; ++bin) {
    const double frequency = low + (high - low) * bin / (bins - 1.0);
    double real = 0.0, imaginary = 0.0;
    for (std::size_t i = 0; i < samples.size; ++i) {
      const double angle = 2.0 * pi * frequency * (times[i] - centre);
      real += weights[i] * samples.signal[i] * std::cos(angle);
      imaginary -= weights[i] * samples.signal[i] * std::sin(angle);
    }
    const double power = real * real + imaginary * imaginary;
    if (power > best_power) { best_power = power; best_frequency = frequency; }
  }
  Parameters p{{samples.signal[peak], best_frequency, 0.0,
                 std::max(duration / 6.0, dt), centre}};
  return projected(p, bounds);
}

FitResult Fitter::fit(const FitRequest& request) {
  FitResult result; std::string error;
  if (!validate_request(request, error)) { result.diagnostic = error; return result; }
  const Prepared prepared = prepare_samples(request.samples, times_, weights_);
  Parameters start = request.options.automatic_initialization
    ? automatic_initialization(request.samples, request.bounds) : projected(request.initial, request.bounds);
  StageResult best = two_stage(request, prepared, times_, weights_, start,
    request.options.max_iterations, residual_, trial_residual_, jacobian_);
  result.attempts = 1;
  if (!best.converged && request.options.deterministic_fallback) {
    StageResult continuation = two_stage(request, prepared, times_, weights_, best.parameters,
      request.options.fallback_iterations, residual_, trial_residual_, jacobian_); ++result.attempts;
    if (continuation.converged || continuation.objective < best.objective) best = continuation;
    if (!best.converged) {
      Parameters spectral = automatic_initialization(request.samples, request.bounds);
      StageResult alternate = two_stage(request, prepared, times_, weights_, spectral,
        request.options.fallback_iterations, residual_, trial_residual_, jacobian_); ++result.attempts;
      if (alternate.converged || alternate.objective < best.objective) best = alternate;
    }
  }
  const double energy = signal_energy(request.samples, weights_, prepared.weight_sum);
  const bool near_sigma = std::abs(best.parameters[3] - request.bounds.lower[3]) <= 1e-10 ||
                          std::abs(best.parameters[3] - request.bounds.upper[3]) <= 1e-10;
  const bool near_amp = std::abs(best.parameters[0] - request.bounds.lower[0]) <= 1e-10 ||
                        std::abs(best.parameters[0] - request.bounds.upper[0]) <= 1e-10;
  const bool near_freq = std::abs(best.parameters[1] - request.bounds.lower[1]) <= 1e-10 ||
                         std::abs(best.parameters[1] - request.bounds.upper[1]) <= 1e-10;
  const bool alias = prepared.uniform && best.parameters[1] > 0.5 / prepared.dt * (1.0 + 1e-12);
  result.qc_triggered = request.options.quality_control &&
    (alias || ((near_sigma || (near_amp && near_freq)) && best.objective / std::max(energy, 1e-30) >=
      request.options.qc_unexplained_energy));
  if (result.qc_triggered) {
    Parameters rescue_start = automatic_initialization(request.samples, request.bounds);
    StageResult rescue = two_stage(request, prepared, times_, weights_, rescue_start,
      request.options.fallback_iterations, residual_, trial_residual_, jacobian_); ++result.attempts;
    const double required = std::max(1e-8,
      request.options.qc_required_relative_improvement * best.objective);
    if (rescue.converged && best.objective - rescue.objective > required) {
      best = rescue; result.rescue_accepted = true;
    }
  }
  result.parameters = best.parameters; result.objective = best.objective;
  result.residual_rms = std::sqrt(std::max(0.0, best.objective));
  result.iterations = best.iterations; result.evaluations = best.evaluations;
  result.bound_active = best.bound_active;
  if (!std::isfinite(best.objective) || best.numerical_failure) {
    result.status = Status::numerical_failure; result.diagnostic = "nonfinite or ill-conditioned solver state";
  } else if (best.converged) {
    result.status = Status::converged; result.diagnostic = "converged";
  } else {
    result.status = Status::valid_nonconverged; result.diagnostic = "valid feasible iterate; convergence criteria not met";
  }
  return result;
}
} // namespace gabor

