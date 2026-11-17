#pragma once
#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
# if defined(GABORFIT_BUILDING_LIBRARY)
#  define GABORFIT_API __declspec(dllexport)
# else
#  define GABORFIT_API __declspec(dllimport)
# endif
#else
# define GABORFIT_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define GABORFIT_ABI_VERSION 1u

typedef struct gaborfit_context gaborfit_context;

enum {
  GABORFIT_STATUS_CONVERGED = 0,
  GABORFIT_STATUS_VALID_NONCONVERGED = 1,
  GABORFIT_STATUS_INVALID_ARGUMENT = -1,
  GABORFIT_STATUS_NUMERICAL_FAILURE = -2,
  GABORFIT_STATUS_INTERNAL_ERROR = -3
};

typedef struct {
  uint32_t abi_version;
  uint32_t struct_size;
  const double* signal;
  const double* times;
  const double* weights;
  size_t sample_count;
  double sampling_frequency_hz;
  double initial[5];
  double lower[5];
  double upper[5];
  int32_t max_iterations;
  int32_t fallback_iterations;
  double x_tolerance;
  double f_tolerance;
  double gradient_tolerance;
  double initial_damping;
  int32_t automatic_initialization;
  int32_t deterministic_fallback;
  int32_t quality_control;
} gaborfit_request;

typedef struct {
  uint32_t abi_version;
  uint32_t struct_size;
  int32_t status;
  double parameters[5];
  double objective;
  double residual_rms;
  int32_t iterations;
  int32_t evaluations;
  int32_t attempts;
  int32_t qc_triggered;
  int32_t rescue_accepted;
  int32_t bound_active;
  char diagnostic[256];
} gaborfit_result;

GABORFIT_API uint32_t gaborfit_abi_version(void);
GABORFIT_API const char* gaborfit_backend_name(void);
GABORFIT_API void gaborfit_request_defaults(gaborfit_request* request);
GABORFIT_API gaborfit_context* gaborfit_context_create(size_t reserve_samples);
GABORFIT_API void gaborfit_context_destroy(gaborfit_context* context);
GABORFIT_API int32_t gaborfit_fit(gaborfit_context* context,
                                  const gaborfit_request* request,
                                  gaborfit_result* result);
GABORFIT_API double gaborfit_evaluate(double time_seconds, const double parameters[5]);
GABORFIT_API void gaborfit_evaluate_jacobian(double time_seconds, const double parameters[5],
                                              double jacobian[5]);

#ifdef __cplusplus
}
#endif

