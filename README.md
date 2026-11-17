# GaborFit C++

GaborFit is a standalone C++17 library for bounded five-parameter Gabor fitting:

```text
g(t) = A exp(-(t-t0)^2 / (2 sigma^2)) cos(2 pi f (t-t0) + phase)
```

Parameters use amplitude units, Hz, radians, and seconds. The numerical core has
no MATLAB, Python, or Julia runtime dependency. It supports uniform sampling or
explicit strictly increasing time vectors, arbitrary window lengths, optional
nonnegative sample weights/masks, caller-provided bounds and starts, and reusable
fitting contexts.

The numerical algorithm is frozen at version 0.1.0. It uses analytic derivatives,
two-stage bounded LM/QR, Jacobian scaling, safe projection, deterministic fallback,
and optional reference-independent QC/rescue.

## Build and test

Eigen 3.3 or newer and CMake 3.18 or newer are required.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DGABORFIT_ENABLE_CERES=OFF
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake --install build --prefix /desired/prefix
```

The installed CMake target is `GaborFit::gaborfit`. Public interfaces are:

- C++: `include/gabor/gabor.hpp`
- stable C ABI v1: `include/gabor/gabor_c.h`
- Python ctypes: `python/gaborfit.py`
- experimental MATLAB MEX: `matlab/gabor_fit_mex.cpp`

Reuse one `gabor::Fitter`, `gaborfit_context`, or Python `Fitter` to retain work
buffers. The stable integration boundary is the C ABI; C++ ABI compatibility is
not promised across incompatible compiler or runtime versions.

## Numerical requirements

- At least five finite signal samples and five positive-weight samples.
- Positive finite uniform sampling frequency, or finite strictly increasing times.
- Nonnegative frequency bounds and strictly positive sigma bounds.
- Finite lower bounds strictly below upper bounds.
- Frequencies are not silently clipped to Nyquist. Uniform fits above Nyquist are
  flagged by QC because the generating frequency is not identifiable.

The objective is weighted mean squared residual normalized by total sample weight.

## Continuous integration

CI uses synthetic signals only. It builds and tests Linux x86-64/GCC, Windows
x86-64/MSVC, macOS Intel/Clang, and macOS Apple Silicon/Clang. Tests cover the
C++ and C APIs, shared-library exports/loading, Python ctypes, CMake installation,
sampling rates from 250 to 4,000 Hz, windows from 51 to 801 samples, nonuniform
times, weights, bounds, signed amplitudes, and invalid inputs.
