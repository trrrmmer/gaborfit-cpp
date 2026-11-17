# Parameter guide

This guide distinguishes implemented defaults from practical advice. Except where
explicitly stated, suggested changes are not validated universal EEG ranges and
must be evaluated on the user's own data.

## Cleaner parameters

| C++ `Options` name | MATLAB/Python alias | Default | Unit | Required constraint |
|---|---|---:|---|---|
| `median_filter_duration_s` | same | 0.1 | s | finite, nonnegative; use positive in practice |
| `analysis_window_width_s` | `analysis_window_width_s`; legacy `analyse_window_width_s` | 2.0 | s | finite and positive |
| `minimum_peak_distance_s` | `min_tpeak_diff_s` | 0.1 | s | finite and nonnegative |
| `fitting_window_width_s` | `cutoff_window_width_s` | 0.2 | s | positive and long enough for at least 5 samples |
| `maximum_event_width_s` | `max_event_width_s` | 0.03 | s | finite and positive |
| `pass_count` | `run_count` | 1 | count | integer >=1 |
| `retain_stage_signals` | no historical equivalent | false | boolean | true/false |

The complete cleaner also requires a finite positive sampling rate and at least
five finite samples. The current API diagnoses invalid sampling rate, pass count,
null input, short input, and nonfinite signal values. Callers should enforce the
remaining constraints above rather than relying on behavior for nonsensical
negative durations.

### `median_filter_duration_s`

The running-median width is `floor(duration*fs)+1` samples. The median defines the
slow background removed only for detection/fitting; accepted Gabors are subtracted
from the original signal.

- Start with the implemented 0.1 s default.
- Increase it if the median itself follows and suppresses relatively long artifact
  envelopes. A longer window preserves more transient energy in the high-pass
  target but tracks baseline changes less closely.
- Decrease it when slow baseline variation contaminates detection, provided the
  median does not begin tracking the muscle spike.
- Too short can absorb the artifact into the median and cause under-detection.
- Too long can make slow neural/baseline deflections enter the detection target.

### `minimum_peak_distance_s`

Candidates closer than this distance are iteratively fused, retaining the larger
absolute excursion. It also affects how dense activity is represented before
fitting.

- Start with 0.1 s for isolated spikes. Existing dense-recording development
  comparisons also used 0.015 s; that is evidence for those recordings, not a
  general recommendation.
- Decrease it only when visually distinct neighboring artifacts are being merged.
- Increase it when one oscillatory artifact is represented by several candidates.
- Too small can produce competing or truncated neighboring fits.
- Too large can suppress genuine separate muscle events.

### `fitting_window_width_s`

This is the nominal full Gabor window. Actual sample count is
`2*floor(floor(width*fs)/2)+1`, always odd. Neighbor and recording boundaries are
zero padded. The center and width parameters use seconds relative to this window.

- The implemented and real-data development setting is 0.2 s.
- Increase only if complete accepted artifacts extend beyond the window; also
  review `maximum_event_width_s` and nearby-event behavior.
- Decrease when dense events cause frequent window overlap, provided the entire
  artifact remains covered.
- Too short truncates waveforms and biases width/frequency.
- Too long admits unrelated signal and increases overlap/local-minimum risk.

At any sampling rate the window must contain at least five samples. In practice it
must contain enough cycles to identify frequency and enough baseline around the
Gaussian envelope.

### `maximum_event_width_s`

Before fitting, candidates whose third-zero-crossing span is not strictly less
than this threshold are excluded. After fitting, a Gabor whose absolute waveform
exceeds 10% of its maximum across more than this duration is rejected.

- Default: 0.03 s.
- Development recordings used 0.04–0.06 s for dense cases.
- Increase only after confirming that rejected wider events are genuine target
  muscle spikes rather than neural transients or overlapping activity.
- Decrease to restrict cleaning to brief, sharply localized artifacts.
- Too small causes under-cleaning; too large permits broad subtraction and raises
  over-cleaning risk.

This parameter interacts strongly with fitted sigma and fitting-window width.

### `analysis_window_width_s`

Detection thresholds and complete-linkage amplitude outliers are evaluated in
sliding windows of `round(width*fs)` samples with 75% increments, plus a reversed
tail pass. If the window exceeds the recording length, no normal detection window
is evaluated.

- Default: 2.0 s.
- Shorten when artifact statistics change rapidly or dense and quiet epochs need
  local thresholds.
- Lengthen when sparse isolated spikes need comparison against a larger background
  population, while ensuring the recording contains a complete window.
- Too short may provide too few candidate maxima for stable clustering.
- Too long mixes heterogeneous epochs and can hide moderate artifacts behind dense
  activity.

The development data included recording-specific windows, including 1 s dense
cases. Such values require dataset-specific justification.

### `pass_count`

Each additional pass redetects events after previous accepted fits are subtracted.

- Start with one pass.
- Increase one pass at a time if strong events mask clear residual artifacts.
- Stop when incremental removed signal is negligible or begins to include plausible
  neural activity.
- More passes are not intrinsically better and can amplify differences between
  solver local minima. The dense development benchmark used five passes.

### `retain_stage_signals`

When true, each pass retains its running median and high-pass target for auditing.
It does not change cleaning. It adds approximately two signal-length `double`
vectors per pass and should normally be false in large batches.

## Internal detection settings

The threshold sweep count, quartile factor used in initial candidate enumeration,
third-zero-crossing count, maximum six amplitude clusters, percentile split rule,
75% window increment, 10% fitted-width definition, explained-variance rejection,
and 0.2 center-envelope criterion are fixed parts of the validated algorithm.
They are deliberately not public tuning parameters. Changing them creates a new
algorithm requiring separate validation.

## Gabor solver settings

The complete cleaner uses the frozen fitter settings and does not expose solver
tolerances separately. The standalone `gabor::Fitter` exposes:

| `SolverOptions` field | Default | Meaning |
|---|---:|---|
| `max_iterations` | 100 | primary bounded LM/QR iteration limit |
| `fallback_iterations` | 200 | deterministic fallback limit |
| `x_tolerance` | 1e-5 | scaled parameter-step tolerance |
| `f_tolerance` | 1e-5 | relative objective tolerance |
| `gradient_tolerance` | 1e-8 | scaled gradient tolerance |
| `initial_damping` | 1e-3 | LM damping start |
| `automatic_initialization` | false | derive a start for standalone fitting |
| `deterministic_fallback` | true | enable bounded deterministic rescue |
| `quality_control` | true | enable reference-independent QC |
| `qc_unexplained_energy` | 0.4 | QC unexplained-energy trigger |
| `qc_required_relative_improvement` | 1e-3 | rescue acceptance improvement |

These are advanced numerical controls. Keep the defaults unless running a separate
solver-validation study. Raising evaluation limits without diagnosing scaling,
bounds, or an alternate minimum can waste time without improving scientific fit.

The cleaner derives each event's amplitude sign, frequency, phase, sigma and center
bounds from its target. Users cannot override those per-event bounds through the
cleaner; use the standalone fitter when explicitly controlled bounds are required.

## Sampling rate

All public cleaner durations are seconds and are converted to integer samples by
the formulas above. Therefore changing sampling rate changes rounding, window
parity, and the number of samples spanning zero crossings even when durations are
unchanged. Synthetic tests cover 250, 500, 1000, 2000 and 4000 Hz. The real EEG
development comparison used 1000 Hz, so other rates have numerical/generalization
tests but not equivalent real-data validation.

Do not rescale durations inversely with sampling rate merely to preserve the old
sample count: first preserve the intended physical time, then inspect rounding and
whether the artifact has enough samples and cycles.

## Guidance by recording characteristic

- Sparse isolated spikes: begin with all defaults and one pass. Confirm complete
  waveform coverage and accepted-event localization.
- Dense activity: consider a shorter analysis window and smaller minimum distance
  only after reviewing candidate grouping; inspect overlaps and use additional
  passes cautiously.
- Closely spaced events: jointly review minimum distance and fitting-window width.
  Changing only one can trade candidate fusion for window truncation.
- Low-amplitude artifacts: do not simply loosen width constraints. Inspect whether
  amplitude-outlier clustering can distinguish them locally; component selection
  or dataset-specific optimization may be required.
- Different ICA components: retain component-specific visual review because scale,
  morphology, neural content, and event density vary even though amplitude units
  themselves need no normalization.

## Controlled tuning workflow

1. Freeze a representative development subset and a separate validation subset.
2. Run defaults with one pass; save events, rejected fits, cleaned signal, and
   difference signal.
3. Inspect sparse, dense, boundary, overlapping, and atypical epochs plus spectra.
4. Tune the median duration alone if baseline separation is wrong.
5. Tune the logically linked detector group—analysis width and minimum distance—one
   value at a time while tracking event identities.
6. Tune fitting width and maximum event width together conceptually, but change one
   at a time and inspect rejected/accepted waveform widths.
7. Only then test additional passes and evaluate incremental subtraction per pass.
8. Compare residual artifact activity and potential over-subtraction; do not select
   parameters solely by minimum residual energy.
9. Freeze settings and evaluate once on the untouched validation subset.
