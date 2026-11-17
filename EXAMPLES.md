# Empirical EEG/ICA cleaning examples

Figures 11–16 show one-second intervals from six real ICA-component recordings.
The program cleaned each full 30-second recording first. The one-second view was
then taken from the result. This keeps the normal detection context around each
example.

Each figure has three panels: the original signal, the total fitted subtraction,
and the final cleaned signal. A triangle marks an event accepted by the native
cleaner. The three panels use the same amplitude scale. The subtraction is
exactly `original - final cleaned`. The signals were not smoothed or retouched.

The filenames are kept so that old links still work. All examples were recorded
at 1000 Hz. All source recordings used five cleaning passes.

The figures use neutral recording codes. Source recordings, numerical arrays,
source names, hashes and event-level provenance remain in the private workspace.

## 1. Repeated narrow spikes

W1 contains 17 accepted events. Several narrow spikes rise clearly above a quiet
background, so the fitted subtraction is easy to compare with the input.

![W1: one-second empirical ICA signal with 17 accepted events, cumulative subtraction and final cleaned signal](examples/generated/single_isolated.png)

## 2. Clear events on a quiet background

W2 contains 12 accepted events. The larger spikes are separated well enough to
follow them from the original signal to the subtraction and cleaned signal.

![W2: one-second empirical ICA signal with 12 accepted events and the actual native subtraction](examples/generated/multiple_separated.png)

## 3. A small group of visible spikes

W3 contains nine accepted events. This less crowded view is useful for checking
individual event markers and the local effect of cleaning.

![W3: one-second empirical ICA signal with nine accepted events and shared amplitude axes](examples/generated/closely_spaced.png)

## 4. Mixed event sizes

W4 contains 14 accepted events. It shows that one recording can contain both
clear spikes and smaller accepted events.

![W4: one-second empirical ICA signal with 14 accepted events](examples/generated/sampling_500_hz.png)

## 5. Strong clustered activity

W5 contains 18 accepted events. The original signal has a clear high-amplitude
cluster, while genuine background and residual activity remain visible.

![W5: one-second empirical ICA signal with a strong cluster and actual fitted subtraction](examples/generated/dense_activity.png)

## 6. Several high-contrast spikes

W6 contains 18 accepted events. The main spikes are easy to see in the original
recording, and the common scale shows the size of the actual subtraction.

![W6: one-second empirical ICA signal with several high-contrast accepted events](examples/generated/three_passes.png)

Small residual spike-like components can remain after cleaning. Additional
passes may reduce some residuals when their morphology and the selected
parameters make them eligible, but additional passes do not guarantee
improvement.

The compact command-line example `gaborfit_clean_example` and
`examples/clean_synthetic.py` remain reproducible API smoke tests. Their
synthetic signals are numerical fixtures and are not presented as representative
scientific cleaning results.

Appendix A provides ten more one-second empirical examples selected by visual
review. They include quieter intervals, clustered activity and more difficult
residual patterns.
