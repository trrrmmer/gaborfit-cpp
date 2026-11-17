# Worked synthetic examples

All examples are synthetic and reproducible. Build the library, install NumPy and
Matplotlib, then run:

```bash
PYTHONPATH=python python examples/generate_examples.py
```

The script uses the actual `Cleaner` API and writes six PNG figures plus
`examples/generated/synthetic_examples.csv`. Each figure shows the original
signal with accepted event positions, the total fitted/subtracted waveform, and
the cleaned signal.

## 1. Single isolated spike

`single_isolated.png` places one large 75 Hz, 8 ms-sigma artifact among small
background transients. One pass and a 1 s analysis window illustrate local
amplitude-outlier selection. The 0.2 s fitting window comfortably contains the
synthetic envelope; the 0.06 s maximum width is intentionally wider than the
default for this demonstrative synthetic morphology.

## 2. Multiple separated spikes

`multiple_separated.png` contains three prominent artifacts separated by more
than the 0.04 s minimum distance. It demonstrates that selected events are fitted
and subtracted in deterministic time order within one pass.

## 3. Closely spaced spikes

`closely_spaced.png` places two prominent events 45 ms apart. The minimum distance
is 40 ms, so they remain candidates, while neighbor-aware fitting windows are
split at their midpoint. This example is useful for inspecting truncation and
should not be read as validation for arbitrary overlapping physiological events.

## 4. Dense artifact activity

`dense_activity.png` uses twenty transient Gabors with varying amplitude,
frequency and width. Two passes illustrate redetection after first-pass
subtraction. Dense activity can make the Gabor model or amplitude-outlier detector
inadequate; inspect the difference signal rather than assuming all removed energy
is artifact.

## 5. Different sampling frequency

`sampling_500_hz.png` uses 500 Hz with the same time-based API. Integer sample
windows are recalculated from seconds. The automated tests additionally cover
250, 1000, 2000 and 4000 Hz.

## 6. One versus multiple passes

`three_passes.png` uses three passes so weaker events can be reconsidered after
stronger fits are removed. Change `run_count` to 1 and regenerate the figure to
compare. Multiple passes are a scientific choice, not merely a speed/accuracy
setting; inspect incremental removal before adopting them.

The compact command-line example `gaborfit_clean_example` and
`examples/clean_synthetic.py` provide minimal integrations without figure logic.
