# Third-party notices

GaborFit does not vendor third-party source code in this repository. Its default
CMake configuration downloads the exact Eigen 3.4.0 source archive and verifies
its SHA-256 checksum before compilation.

## Eigen 3.4.0

Eigen is distributed primarily under the Mozilla Public License 2.0. Individual
Eigen files may carry additional compatible license notices. Eigen's own source
headers and accompanying license files govern those files; the GaborFit
BSD-3-Clause license does not replace them.

- Project: <https://eigen.tuxfamily.org/>
- License: <https://www.mozilla.org/MPL/2.0/>
- Pinned archive checksum:
  `8586084f71f9bde545ee7fa6d00288b264a2b7ac3607b974e54d13e7162c1c72`

## Optional integration dependencies

Python/NumPy and MATLAB are optional interface environments and are not bundled.
Ceres may be detected by CMake for capability reporting, but version 0.2.0 keeps
the validated Eigen bounded-LM/QR backend. Users are responsible for complying
with the licenses of optional tools they install separately.
