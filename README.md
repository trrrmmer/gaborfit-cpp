# GaborFit EEG Muscle Artifact Removal

GaborFit is a native C++17 library that detects short muscle-artifact spikes in
single-channel EEG or ICA-component time series, fits bounded one-dimensional
Gabor waveforms, and subtracts accepted fits. It also exposes the Gabor fitter as
an independent general-purpose API. MATLAB, Python and Julia are not required at
runtime.

The software targets transient, oscillatory EMG spikes. It is not a replacement
for ICA decomposition, channel rejection, or clinical review, and it should not
be applied blindly to ordinary cerebral high-frequency activity.

## Highlights

- Complete one-pass or multi-pass native cleaning.
- Frozen analytic bounded LM/QR Gabor fitter with deterministic fallback.
- C++ API, stable C ABI v1, Python `ctypes`, and experimental MATLAB MEX.
- Sampling-rate-independent parameters expressed in seconds and hertz.
- Per-pass and per-event diagnostics, including rejected fits.
- CMake builds for Linux, Windows, macOS Intel, and macOS Apple Silicon.
- Cross-platform CI and the public illustrated manual use synthetic,
  non-identifying data only. No empirical EEG arrays or figures are included.

## Illustrated user manual

The [illustrated English user manual](docs/manual/GaborFit_User_Manual_0.2.0.pdf)
covers installation, native C++, Python `ctypes`, MATLAB MEX and command-line
use, parameter selection, troubleshooting, scientific limitations and
reproducible synthetic cleaning examples.

| Platform | Native build | Interfaces |
|---|---|---|
| Linux x86-64 | GCC/Clang with CMake | C++, C ABI, Python `ctypes`, MATLAB MEX |
| Windows x86-64 | MSVC with CMake | C++, C ABI, Python `ctypes`, MATLAB MEX |
| macOS Intel x86-64 | Apple Clang with CMake | C++, C ABI, Python `ctypes`, MATLAB MEX |
| macOS Apple Silicon ARM64 | Apple Clang with CMake | C++, C ABI, Python `ctypes`, MATLAB MEX |

MATLAB support is experimental and each MATLAB platform requires a locally
compiled MEX binary. Python and MATLAB are optional; the native library has no
runtime dependency on either environment.

## Quick start

Requirements: CMake 3.18+, a C++17 compiler, and either internet access for the
pinned Eigen 3.4.0 archive or an installed exact Eigen 3.4.0 package.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DGABORFIT_ENABLE_CERES=OFF
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
./build/gaborfit_clean_example       # Windows: build\Release\gaborfit_clean_example.exe
```

Minimal C++ use:

```cpp
#include <gabor/eeg_clean.hpp>
gabor::eeg::Cleaner cleaner(signal.size());
gabor::eeg::Options options;
options.pass_count = 1;
auto result = cleaner.clean(signal.data(), signal.size(), sampling_rate_hz, options);
if (!result.valid) throw std::runtime_error(result.diagnostic);
const std::vector<double>& cleaned = result.cleaned;
```

The default cleaner preserves input length and ordering. Native diagnostic sample
indices are zero-based and windows are inclusive.

## Documentation

- [Installation on each platform](INSTALL.md)
- [Practical user guide](USER_GUIDE.md)
- [Parameter selection and tuning](PARAMETER_GUIDE.md)
- [Reproducible synthetic examples](EXAMPLES.md)
- [API and ABI reference](API.md)
- [Scientific and technical limitations](SCIENTIFIC_LIMITATIONS.md)
- [Illustrated empirical user manual](docs/manual/GaborFit_User_Manual_0.2.0.pdf)

The accepted development benchmark used six real 30,000-sample ICA recordings
and measured a 26.4× aggregate end-to-end speedup over corrected MATLAB on the
same Linux workstation. This is a workload-specific development result, not a
cross-platform performance guarantee or independent clinical validation.

## Version and scientific status

Version `0.2.0` packages the frozen Gabor engine and frozen complete cleaner.
Different optimizers may produce different local minima even when waveforms are
scientifically comparable. Greater removed energy is not proof of better
cleaning. Inspect event diagnostics, spectra, and representative time-domain
segments before using results scientifically.

The independently developed C++ code is distributed under the
[BSD 3-Clause License](LICENSE). Third-party dependencies remain subject to
their own licenses; see [third-party notices](THIRD_PARTY_NOTICES.md).
