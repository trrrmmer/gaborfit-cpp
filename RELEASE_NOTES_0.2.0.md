# GaborFit 0.2.0 release notes

## Scope

Version 0.2.0 packages the frozen standalone five-parameter Gabor fitter and the
complete native C++17 EEG muscle-artifact cleaning engine. It does not alter the
validated fitting, detection, QC, fallback, scaling, bounds or subtraction
algorithms.

## Included interfaces

- Native C++ fitting and complete-cleaning APIs.
- Stable C ABI version 1 for fitting and complete cleaning.
- Python `ctypes` wrappers.
- Experimental MATLAB MEX wrappers.
- CMake build, install and installed-package consumer support.

## Cross-platform validation

Hosted GitHub Actions run `44064821667` passed the native fitter tests on Linux
x86-64, Windows x86-64, macOS Intel x86-64 and macOS Apple Silicon ARM64. The
complete cleaner is covered by the current four-platform workflow. A release is
not considered validated until that workflow also passes on the final release
commit.

The CI workload uses synthetic data only. It verifies compilation, installation,
native C++ tests, C ABI exports and loading, Python wrappers, variable sampling
rates and window lengths, bounds, one-pass and multi-pass cleaning and
deterministic output.

## Performance evidence

The development benchmark measured approximately 26.4× aggregate end-to-end
acceleration over corrected MATLAB for six 30,000-sample ICA-component
recordings on one Linux workstation. This workload-specific result is not an
independent clinical validation or a cross-platform performance guarantee.

## Documentation

The illustrated English manual documents Linux, Windows, macOS Intel and macOS
ARM64 installation; native C++, C, Python and MATLAB use; parameter selection;
worked examples; troubleshooting; and scientific limitations. The public manual
uses synthetic, non-identifying examples. Empirical EEG figures, raw arrays,
participant identifiers and internal provenance are not part of the repository
or release assets.

## Scientific limitations

- The cleaner targets transient muscle-artifact spikes and is not a replacement
  for ICA decomposition, channel rejection or expert review.
- Greater removed energy or lower high-frequency power is not by itself evidence
  of better cleaning.
- Dense and overlapping activity can produce different multi-pass trajectories
  from corrected MATLAB.
- Parameter choices require representative visual and spectral review before
  controlled use on a new dataset.
- The reported empirical benchmarks are development/regression evidence, not an
  independent prospective validation.

## Licensing and data rights

The independently developed C++ code is released under BSD-3-Clause, subject to
the licenses of third-party dependencies. Empirical EEG recordings and derived
empirical appendix figures are excluded from the public release.
