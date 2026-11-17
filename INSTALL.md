# Installation

## Requirements

Mandatory:

- CMake 3.18 or newer.
- A C++17 compiler.
- Eigen 3.4.0. By default CMake downloads a pinned source archive and checks
  SHA-256 `8586084f71f9bde545ee7fa6d00288b264a2b7ac3607b974e54d13e7162c1c72`.

Optional:

- Python 3.9+ and NumPy for the `ctypes` wrapper and Python examples.
- MATLAB with a configured supported C++ MEX compiler for the experimental MEX.
- Ceres may be detected but is not used as the reproducibility backend. Configure
  with `GABORFIT_ENABLE_CERES=OFF` for the validated build.

An offline build may use an installed exact Eigen 3.4.0 with
`-DGABORFIT_USE_SYSTEM_EIGEN=ON`.

## Linux

Use GCC or Clang:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DGABORFIT_ENABLE_CERES=OFF
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --install build --prefix "$HOME/.local"
```

At runtime, make the installed library discoverable if the prefix is not on the
loader path, for example `export LD_LIBRARY_PATH="$HOME/.local/lib:$LD_LIBRARY_PATH"`.

## Windows x86-64

Open an x64 Native Tools Command Prompt for Visual Studio 2022:

```powershell
cmake -S . -B build -A x64 -DGABORFIT_ENABLE_CERES=OFF
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
cmake --install build --config Release --prefix C:\gaborfit
```

Add `C:\gaborfit\bin` to `PATH` when loading the DLL. The tested compiler target
is MSVC x64; MinGW binary compatibility is not implied.

## macOS Intel

Run in an x86-64 terminal with Apple Clang and an Intel CMake installation:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=x86_64 -DGABORFIT_ENABLE_CERES=OFF
cmake --build build --parallel
ctest --test-dir build --output-on-failure
sudo cmake --install build --prefix /usr/local
```

## macOS Apple Silicon

Use a native ARM64 terminal:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DGABORFIT_ENABLE_CERES=OFF
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --install build --prefix "$HOME/.local"
```

Do not mix x86-64 Python or MATLAB with an ARM64 native library, or vice versa.

## CMake consumer

After installation:

```cmake
find_package(GaborFit CONFIG REQUIRED)
target_link_libraries(my_cleaner PRIVATE GaborFit::eeg_clean)
```

Use `GaborFit::gaborfit` for isolated Gabor fitting. Include files are installed
under `include/gabor`.

## Python

The wrapper depends only on NumPy and the shared libraries:

```bash
python -m pip install numpy
PYTHONPATH=python python examples/clean_synthetic.py
```

If the library is not in the repository `build` directory, pass it explicitly:

```python
cleaner = Cleaner("/absolute/path/to/libgaborfit_eeg.so")
```

Use `.dll` on Windows and `.dylib` on macOS. The dependent standalone
`gaborfit` library must be in the same directory or on the platform loader path.

## MATLAB MEX

First configure a supported compiler with `mex -setup C++`, then build the CMake
library and run:

```matlab
addpath('matlab');
build_eeg_clean_mex(fullfile(pwd,'build'));
[cleaned, diagnostics] = eeg_clean_mex(signal, sampling_rate_hz, options);
```

On Windows pass `fullfile(pwd,'build')`; the script automatically locates the
`Release` DLL. The MEX is experimental and must match MATLAB's architecture.
MATLAB on Linux bundles its own C++ runtime; use MathWorks' supported compiler.
If a MEX reports a missing `GLIBCXX` version, rebuild with a supported compiler
instead of replacing MATLAB's bundled libraries globally.

## Build switches

| Option | Default | Purpose |
|---|---:|---|
| `GABORFIT_BUILD_SHARED` | ON | Build the standalone fitter shared library. |
| `GABORFIT_BUILD_EEG_CLEANER` | ON | Build the complete cleaner shared library. |
| `GABORFIT_BUILD_TESTS` | ON | Build native regression tests. |
| `GABORFIT_BUILD_EXAMPLES` | ON | Build the command-line example. |
| `GABORFIT_USE_SYSTEM_EIGEN` | OFF | Require installed exact Eigen 3.4.0. |
| `GABORFIT_ENABLE_CERES` | ON | Detect Ceres; validated commands disable it. |
