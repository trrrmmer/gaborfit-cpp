"""Synthetic end-to-end smoke tests for the one-call EEG-cleaning C ABI."""
from __future__ import annotations
import platform
from pathlib import Path
import numpy as np
from eeg_clean import Cleaner


def library_path() -> Path:
    root = Path(__file__).resolve().parents[1]
    system = platform.system()
    if system == "Windows":
        return root / "build" / "Release" / "gaborfit_eeg.dll"
    return root / "build" / ("libgaborfit_eeg.dylib" if system == "Darwin" else "libgaborfit_eeg.so")


def signal(fs: float, seconds: float = 3.0) -> np.ndarray:
    count = int(round(fs * seconds))
    time = np.arange(count) / fs
    value = 0.03 * np.sin(2 * np.pi * 9 * time)
    for center, amplitude in ((0.6, 1.0), (1.2, 4.0), (1.8, 1.2), (2.4, 0.9)):
        dt = time - center
        value += amplitude * np.exp(-0.5 * (dt / 0.008) ** 2) * np.cos(2 * np.pi * 70 * dt + 0.2)
    return value


def main() -> None:
    cleaner = Cleaner(library_path(), reserve_samples=12000)
    for fs in (250.0, 500.0, 1000.0, 2000.0, 4000.0):
        source = signal(fs)
        settings = dict(analysis_window_width_s=1.0,
                        min_tpeak_diff_s=0.04,
                        cutoff_window_width_s=0.2,
                        max_event_width_s=0.06,
                        run_count=2)
        first, diagnostics = cleaner.clean(source, fs, **settings)
        second, repeated = cleaner.clean(source, fs, **settings)
        assert first.shape == source.shape and np.isfinite(first).all()
        assert np.array_equal(first, second), "cleaning is not deterministic"
        assert diagnostics["pass_count"] == repeated["pass_count"] == 2
        assert diagnostics["failed_events"] == 0
        assert diagnostics["attempted_events"] > 0 and diagnostics["accepted_events"] > 0
    print("Python one-call EEG cleaner tests passed")


if __name__ == "__main__":
    main()
