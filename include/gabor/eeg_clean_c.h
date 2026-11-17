#pragma once
#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
# if defined(GABORFIT_EEG_BUILDING_LIBRARY)
#  define GABORFIT_EEG_API __declspec(dllexport)
# else
#  define GABORFIT_EEG_API __declspec(dllimport)
# endif
#else
# define GABORFIT_EEG_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define GABORFIT_EEG_ABI_VERSION 1u
typedef struct gaborfit_eeg_context gaborfit_eeg_context;

typedef struct {
  uint32_t abi_version, struct_size;
  double median_filter_duration_s, analysis_window_width_s;
  double minimum_peak_distance_s, fitting_window_width_s, maximum_event_width_s;
  int32_t pass_count, retain_stage_signals;
} gaborfit_eeg_options;

typedef struct {
  uint32_t abi_version, struct_size;
  int32_t status;
  size_t sample_count, pass_count, attempted_events, accepted_events, failed_events;
  char diagnostic[256];
} gaborfit_eeg_result;

typedef struct {
  size_t pass_index, peak_index;
  int64_t window_start, window_end;
  double initial[5], lower[5], upper[5], parameters[5];
  int32_t solver_status, rejection, accepted, iterations, evaluations, attempts;
  int32_t qc_triggered, rescue_accepted, bound_active;
  double objective, explained_variance;
} gaborfit_eeg_event;

typedef struct {
  size_t pass_index, threshold_peak_count, duration_valid_peak_count;
  size_t selected_peak_count, attempted_event_count, accepted_event_count;
} gaborfit_eeg_pass;

GABORFIT_EEG_API uint32_t gaborfit_eeg_abi_version(void);
GABORFIT_EEG_API void gaborfit_eeg_options_defaults(gaborfit_eeg_options* options);
GABORFIT_EEG_API gaborfit_eeg_context* gaborfit_eeg_context_create(size_t reserve_samples);
GABORFIT_EEG_API void gaborfit_eeg_context_destroy(gaborfit_eeg_context* context);
GABORFIT_EEG_API int32_t gaborfit_eeg_clean(gaborfit_eeg_context* context,
    const double* signal, size_t sample_count, double sampling_frequency_hz,
    const gaborfit_eeg_options* options, double* cleaned, gaborfit_eeg_result* result);
GABORFIT_EEG_API size_t gaborfit_eeg_last_event_count(const gaborfit_eeg_context* context);
GABORFIT_EEG_API int32_t gaborfit_eeg_last_event(const gaborfit_eeg_context* context,
    size_t event_index, gaborfit_eeg_event* event);
GABORFIT_EEG_API size_t gaborfit_eeg_last_pass_count(const gaborfit_eeg_context* context);
GABORFIT_EEG_API int32_t gaborfit_eeg_last_pass(const gaborfit_eeg_context* context,
    size_t pass_index, gaborfit_eeg_pass* pass);

#ifdef __cplusplus
}
#endif
