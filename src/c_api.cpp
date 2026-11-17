#include "gabor/gabor_c.h"
#include "gabor/gabor.hpp"
#include <algorithm>
#include <cstring>
#include <new>

struct gaborfit_context { explicit gaborfit_context(size_t n) : fitter(n) {} gabor::Fitter fitter; };

extern "C" {
uint32_t gaborfit_abi_version(void) { return GABORFIT_ABI_VERSION; }
const char* gaborfit_backend_name(void) { return gabor::Fitter::backend_name(); }
void gaborfit_request_defaults(gaborfit_request* request) {
  if (!request) return;
  std::memset(request,0,sizeof(*request)); request->abi_version=GABORFIT_ABI_VERSION;
  request->struct_size=sizeof(*request); request->max_iterations=100;
  request->fallback_iterations=200; request->x_tolerance=1e-5; request->f_tolerance=1e-5;
  request->gradient_tolerance=1e-8; request->initial_damping=1e-3;
  request->deterministic_fallback=1; request->quality_control=1;
}
gaborfit_context* gaborfit_context_create(size_t n) {
  try { return new gaborfit_context(n); } catch (...) { return nullptr; }
}
void gaborfit_context_destroy(gaborfit_context* context) { delete context; }
double gaborfit_evaluate(double time, const double parameters[5]) {
  if (!parameters) return 0.0;
  gabor::Parameters p{};
  std::copy(parameters, parameters + 5, p.begin()); return gabor::Fitter::evaluate(time, p);
}
void gaborfit_evaluate_jacobian(double time, const double parameters[5], double output[5]) {
  if (!parameters || !output) return;
  gabor::Parameters p{}; std::copy(parameters, parameters + 5, p.begin());
  const auto values=gabor::Fitter::jacobian(time,p); std::copy(values.begin(), values.end(), output);
}
int32_t gaborfit_fit(gaborfit_context* context, const gaborfit_request* input, gaborfit_result* output) {
  if (!context || !input || !output) return static_cast<int32_t>(gabor::Status::invalid_argument);
  std::memset(output, 0, sizeof(*output));
  output->abi_version=GABORFIT_ABI_VERSION; output->struct_size=sizeof(*output);
  if (input->abi_version != GABORFIT_ABI_VERSION || input->struct_size < sizeof(gaborfit_request)) {
    output->status=GABORFIT_STATUS_INVALID_ARGUMENT;
    std::strncpy(output->diagnostic,"incompatible C ABI request structure",sizeof(output->diagnostic)-1);
    return output->status;
  }
  try {
    gabor::FitRequest request;
    request.samples = {input->signal, input->times, input->weights, input->sample_count,
                       input->sampling_frequency_hz};
    std::copy(input->initial, input->initial + 5, request.initial.begin());
    std::copy(input->lower, input->lower + 5, request.bounds.lower.begin());
    std::copy(input->upper, input->upper + 5, request.bounds.upper.begin());
    request.options.max_iterations = input->max_iterations > 0 ? input->max_iterations : 100;
    request.options.fallback_iterations = input->fallback_iterations > 0 ? input->fallback_iterations : 200;
    request.options.x_tolerance = input->x_tolerance > 0 ? input->x_tolerance : 1e-5;
    request.options.f_tolerance = input->f_tolerance > 0 ? input->f_tolerance : 1e-5;
    request.options.gradient_tolerance = input->gradient_tolerance > 0 ? input->gradient_tolerance : 1e-8;
    request.options.initial_damping = input->initial_damping > 0 ? input->initial_damping : 1e-3;
    request.options.automatic_initialization = input->automatic_initialization != 0;
    request.options.deterministic_fallback = input->deterministic_fallback != 0;
    request.options.quality_control = input->quality_control != 0;
    const gabor::FitResult result = context->fitter.fit(request);
    output->status = static_cast<int32_t>(result.status);
    std::copy(result.parameters.begin(), result.parameters.end(), output->parameters);
    output->objective = result.objective; output->residual_rms = result.residual_rms;
    output->iterations = result.iterations; output->evaluations = result.evaluations;
    output->attempts = result.attempts; output->qc_triggered = result.qc_triggered;
    output->rescue_accepted = result.rescue_accepted; output->bound_active = result.bound_active;
    std::strncpy(output->diagnostic, result.diagnostic.c_str(), sizeof(output->diagnostic) - 1);
    return output->status;
  } catch (const std::exception& exception) {
    output->status = static_cast<int32_t>(gabor::Status::internal_error);
    std::strncpy(output->diagnostic, exception.what(), sizeof(output->diagnostic) - 1);
    return output->status;
  } catch (...) {
    output->status = static_cast<int32_t>(gabor::Status::internal_error);
    std::strncpy(output->diagnostic, "unknown native exception", sizeof(output->diagnostic) - 1);
    return output->status;
  }
}
}
