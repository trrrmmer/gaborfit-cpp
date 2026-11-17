#include "gabor/gabor_c.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static int require_true(int value, const char* message) {
  if (!value) fprintf(stderr, "C ABI TEST FAILURE: %s\n", message);
  return value ? 0 : 1;
}

int main(void) {
  enum { count = 201 };
  double signal[count];
  const double truth[5] = {3.0, 80.0, 0.35, 0.014, 0.1};
  for (size_t i = 0; i < count; ++i)
    signal[i] = gaborfit_evaluate((double)i / 1000.0, truth);

  if (require_true(gaborfit_abi_version() == GABORFIT_ABI_VERSION, "ABI version mismatch")) return 1;
  if (require_true(gaborfit_backend_name() != NULL, "missing backend name")) return 1;

  gaborfit_request request;
  gaborfit_request_defaults(&request);
  request.signal = signal;
  request.sample_count = count;
  request.sampling_frequency_hz = 1000.0;
  const double initial[5] = {2.8, 82.0, 0.2, 0.016, 0.1};
  const double lower[5] = {0.5, 5.0, -3.141592653589793, 0.002, 0.07};
  const double upper[5] = {5.0, 300.0, 3.141592653589793, 0.05, 0.13};
  memcpy(request.initial, initial, sizeof(initial));
  memcpy(request.lower, lower, sizeof(lower));
  memcpy(request.upper, upper, sizeof(upper));

  gaborfit_context* context = gaborfit_context_create(count);
  if (require_true(context != NULL, "context creation failed")) return 1;
  gaborfit_result result;
  const int32_t status = gaborfit_fit(context, &request, &result);
  gaborfit_context_destroy(context);
  if (require_true(status == GABORFIT_STATUS_CONVERGED, result.diagnostic)) return 1;
  if (require_true(result.abi_version == GABORFIT_ABI_VERSION, "result ABI mismatch")) return 1;
  if (require_true(result.struct_size == sizeof(gaborfit_result), "result structure size mismatch")) return 1;
  if (require_true(result.objective < 1e-10, "fit objective is inaccurate")) return 1;
  if (require_true(result.parameters[1] >= lower[1] && result.parameters[1] <= upper[1], "bounds violated")) return 1;

  request.abi_version = GABORFIT_ABI_VERSION + 1;
  if (require_true(gaborfit_fit(NULL, &request, &result) == GABORFIT_STATUS_INVALID_ARGUMENT,
                   "null context was accepted")) return 1;
  printf("C ABI smoke test passed; sizeof(request)=%zu sizeof(result)=%zu\n",
         sizeof(gaborfit_request), sizeof(gaborfit_result));
  return 0;
}
