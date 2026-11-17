# Empirical EEG/ICA cleaning examples

Figures 11–16 use genuine, contiguous two-second ICA-component intervals from
six validated benchmark recordings. The frozen native C++ cleaner was run on
each complete 30-second recording before the displayed interval was extracted,
preserving the detector's continuous-recording context.

Every figure shows the empirical input with genuine accepted-event markers,
cumulative fitted subtraction computed as `original - final cleaned`, and the
final cleaned signal. All three panels in an example use identical amplitude
limits and span exactly 0–2 s. No waveform, event, residual, or fitted curve was
added, smoothed, retouched, or independently rescaled.

The six established PNG filenames are retained for compatibility with existing
links. Figure headers state the actual sampling frequency, pass count, event
count, and amplitude range. In particular, the legacy filename
`sampling_500_hz.png` now contains a 1000 Hz empirical example, and
`three_passes.png` reports the actual five full-recording passes.

The rendered figures use neutral recording codes. Original recordings, numerical
arrays, participant and source-recording identifiers, source hashes, and
event-level provenance remain outside this repository.

## 1. Distributed moderate-amplitude events

W1 contains 15 accepted events with comparable amplitudes distributed across the
two-second view. No single onset event controls the amplitude scale.

![W1: empirical two-second ICA signal, accepted events, cumulative fitted subtraction and final cleaned signal](examples/generated/single_isolated.png)

## 2. Separated accepted events

W2 contains seven well-separated events from a three-pass full-recording run.
The wider spacing makes individual fitted subtractions easy to inspect.

![W2: seven separated empirical events and their actual native subtraction](examples/generated/multiple_separated.png)

## 3. Varied but visually separable events

W3 contains 13 events of broadly similar scale on realistic background activity.
Small residual activity remains visible without dominating the presentation.

![W3: empirical accepted events with shared original, subtraction and cleaned amplitude axes](examples/generated/closely_spaced.png)

## 4. Lower-amplitude empirical activity

W4 contains 11 lower-amplitude events. Its narrower common amplitude range shows
that the cleaner and diagnostic display are not restricted to large artifacts.

![W4: lower-amplitude empirical events shown over exactly two seconds](examples/generated/sampling_500_hz.png)

## 5. Moderately frequent transient activity

W5 contains 17 accepted events from a three-pass full-recording run. Events are
frequent but remain individually recognizable.

![W5: moderately frequent empirical transient activity and actual fitted subtraction](examples/generated/dense_activity.png)

## 6. Higher event count without a dominating onset artifact

W6 contains 24 accepted events distributed across the view. It provides the
highest event count in the worked examples while retaining readable individual
spikes and a balanced shared amplitude scale.

![W6: higher-count empirical activity with readable accepted events and final cleaning](examples/generated/three_passes.png)

Small residual spike-like components can remain after cleaning. Additional
passes may reduce some residuals when their morphology and selected parameters
make them eligible, but additional passes do not guarantee improvement.

## Synthetic numerical fixtures

Synthetic signals remain available strictly for reproducible interface and
regression testing:

```bash
PYTHONPATH=python python examples/generate_examples.py
```

The script writes clearly labelled outputs under
`examples/generated/synthetic_test_outputs/`, isolated from the approved
empirical PNG files. It cannot overwrite the public scientific examples. The
compact command-line example `gaborfit_clean_example` and
`examples/clean_synthetic.py` likewise remain API smoke tests without
scientific-performance claims.
