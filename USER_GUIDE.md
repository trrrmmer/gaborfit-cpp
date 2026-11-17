# User guide

## 1. Input data

The complete cleaner accepts one uniformly sampled real-valued time series at a
time. Supply finite `double` values in chronological order and a finite positive
sampling frequency in hertz. Output has exactly the same number and order of
samples. Signal amplitude units are arbitrary but must be internally consistent;
fitted amplitude and residual values use the same unit.

Process continuous single channels or individual ICA-component activations. For
multichannel data, call the cleaner separately for each selected channel or
component. The library does not compute ICA, choose components, rereference data,
or replace bad channels.

The detector performs its own running-median subtraction for detection and
fitting. Subtraction is applied to the supplied original signal. Do not assume
that an external high-pass filter is required, and avoid applying an undocumented
filter only to one side of a comparison.

## 2. C++ example

```cpp
#include <gabor/eeg_clean.hpp>
#include <stdexcept>
#include <vector>

std::vector<double> remove_muscle_spikes(const std::vector<double>& input,
                                         double sampling_rate_hz) {
  gabor::eeg::Options options;
  options.pass_count = 1;
  options.retain_stage_signals = false;
  gabor::eeg::Cleaner cleaner(input.size());
  auto result=cleaner.clean(input.data(),input.size(),sampling_rate_hz,options);
  if(!result.valid) throw std::runtime_error(result.diagnostic);
  for(const auto& pass:result.passes)
    for(const auto& event:pass.events)
      if(event.accepted) {
        // event.peak_index and window indices are zero-based.
      }
  return std::move(result.cleaned);
}
```

Reuse one `Cleaner` for similarly sized signals to reuse the fitter workspace.
The call is deterministic for identical input and options.

## 3. Python example

```python
import numpy as np
from eeg_clean import Cleaner

signal=np.ascontiguousarray(signal,dtype=np.float64)
cleaner=Cleaner(reserve_samples=signal.size)
cleaned,info=cleaner.clean(signal,1000.0,run_count=1)
for event in info["events"]:
    if event["accepted"]:
        print(event["pass"],event["peak_index"],event["parameters"])
```

Input must be one-dimensional. The wrapper makes a contiguous `float64` copy if
needed. Returned arrays are NumPy `float64`. Index fields are zero-based.

## 4. MATLAB example

```matlab
options=struct('median_filter_duration_s',0.1, ...
               'analysis_window_width_s',2.0, ...
               'min_tpeak_diff_s',0.1, ...
               'cutoff_window_width_s',0.2, ...
               'max_event_width_s',0.03, ...
               'run_count',1);
[cleaned,summary]=eeg_clean_mex(double(component(:)),1000,options);
```

The MEX preserves row/column shape and returns aggregate counts. Detailed events
are available through C++/C/Python but are not currently returned by the MEX.

## 5. Passes and event processing

One pass computes a running median, detects positive and negative excursions,
suppresses nearby candidates, measures third-zero-crossing width, and selects
amplitude outliers within sliding analysis windows. Selected events are processed
in ascending sample order. Neighboring fitting windows are split at their
midpoint and zero padded; accepted Gabor waveforms are clipped to signal bounds
and subtracted.

All fits in one pass use the high-pass target frozen at the start of that pass.
With multiple passes, the median and detector are recomputed from the previous
pass's cleaned result. Later passes can therefore detect events that were masked
by stronger activity. Begin with one pass; use additional passes only after
reviewing incremental removal.

## 6. Diagnostics

`Result::passes` contains threshold candidates, duration-valid candidates,
selected peaks, and events. Set `retain_stage_signals=true` to also retain the
running median and high-pass vectors; this roughly adds two signal-length vectors
per pass.

An attempted event may be rejected because its problem is invalid, its fitted
waveform exceeds the maximum width, explained variance is negative, the Gaussian
envelope is too eccentric, or the solver fails numerically. Rejected fits are not
subtracted. `valid_nonconverged` is a finite bounded solver result, distinct from
a numerical failure; inspect objective, waveform, and rejection status.

The five fitted values are amplitude, frequency in hertz, phase in radians,
Gaussian sigma in seconds, and center in seconds relative to the fitting window.

## 7. Exporting results

The library deliberately does not impose a file format. Copy `result.cleaned`
to your existing EDF/MAT/NumPy/HDF5 writer. When exporting diagnostics, retain:
sampling frequency, options, pass and peak indices, physical bounds, fitted
parameters, objective, solver status, rejection, and acceptance. Save enough
precision to round-trip `double` values.

## 8. Scientific inspection

Review original, cleaned, and difference signals around accepted events. Compare
power spectra in bands relevant to the experiment and inspect ordinary neural
transients that should remain. Useful warning signs are:

- Under-cleaning: obvious oscillatory spikes remain; high-frequency bursts remain
  concentrated at detected events; many events are rejected as too wide.
- Over-cleaning: ordinary evoked transients are attenuated; difference signals
  contain sustained or physiologically plausible activity; later passes remove
  progressively broad signal; high-frequency power falls outside artifact epochs.

Lower residual energy alone does not prove better cleaning. Maintain an unmodified
copy of the input and document component selection and parameters.

## 9. Boundaries and unsupported operations

The complete cleaner supports uniform sampling only. The standalone Gabor fitter
supports explicit nonuniform times and weights. Missing samples must not be NaN;
handle gaps before cleaning or process valid continuous segments separately.
There is no built-in multithreaded batch scheduler, resampler, file reader, ICA,
or automatic parameter optimizer.
