# API summary

## C++

Include `<gabor/eeg_clean.hpp>` and link `GaborFit::eeg_clean`. `Cleaner::clean`
accepts a contiguous `double` vector, its length, a positive sampling frequency,
and `Options`. It returns `Result`, containing the length-preserving cleaned
vector and `PassDiagnostics`. Each `Event` records zero-based peak/window indices,
starts, physical bounds, fitted parameters, objective, solver status, rejection,
QC, rescue, and acceptance.

Include `<gabor/gabor.hpp>` and link `GaborFit::gaborfit` for isolated Gabor
fitting. Parameter order is `[amplitude, frequency_hz, phase_rad, sigma_s,
centre_s]`. Explicit nonuniform time vectors and sample weights are supported by
the standalone fitter, not by the complete EEG detector.

## C ABI v1

Include `<gabor/eeg_clean_c.h>`. Create one persistent
`gaborfit_eeg_context`, call `gaborfit_eeg_options_defaults`, then call
`gaborfit_eeg_clean` once per signal. The caller allocates `sample_count`
`double` values for the output. Retrieve the most recent events and passes with
`gaborfit_eeg_last_event*` and `gaborfit_eeg_last_pass*`. The context owns those
diagnostics until the next cleaning call.

The standalone fitting ABI remains in `<gabor/gabor_c.h>`. Both ABIs use fixed
width status fields, explicit structure sizes, and caller-owned signal arrays.
C++ ABI compatibility across different compiler runtimes is not promised; use
the C ABI for binary plugins.

## Status and rejection codes

Solver status: `0` converged, `1` valid nonconverged, `-1` invalid argument,
`-2` numerical failure, `-3` internal error. Cleaner rejection codes are:
`0` accepted, `1` invalid problem, `2` waveform too wide, `3` negative explained
variance, `4` eccentric envelope, and `5` numerical failure.
