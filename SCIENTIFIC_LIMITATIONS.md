# Scientific and technical limitations

- The cleaner targets short oscillatory muscle spikes in single-channel or ICA
  time series. It does not detect every EMG morphology and does not compute ICA.
- A Gabor waveform can fit non-artifact neural transients. Component selection,
  time-domain inspection and spectral review remain the user's responsibility.
- Overlapping and dense events are handled deterministically through candidate
  suppression and midpoint window truncation, but the single-Gabor model can be
  inadequate in those conditions.
- More passes and greater removed energy do not imply better cleaning. The native
  development output removed somewhat more high-frequency power than corrected
  MATLAB in dense recordings.
- Real-data development validation used 1000 Hz signals. Other sampling rates are
  covered by synthetic numerical tests, not equivalent physiological validation.
- The measured 26.4× speedup is specific to six 30,000-sample recordings on one
  Linux workstation and has not been independently reproduced across platforms.
- The MATLAB MEX is experimental and returns aggregate rather than full event
  diagnostics. Linux MATLAB/compiler C++ runtime compatibility requires care.
- The C ABI is stable at version 1. The C++ ABI depends on compiler and standard
  library compatibility.

Before controlled deployment, freeze parameters, validate on representative data
not used for tuning, preserve uncleaned inputs, and document all rejected fits and
outliers. This software is a research tool, not a medical device.
