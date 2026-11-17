"""Minimal executable Python example using the one-call native cleaner."""
from pathlib import Path
import sys
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"python"))
from eeg_clean import Cleaner

fs=1000.0
t=np.arange(3000)/fs
signal=.03*np.sin(2*np.pi*9*t)
for center in (.55,1.2,1.85,2.45):
    dt=t-center
    signal+=3*np.exp(-.5*(dt/.008)**2)*np.cos(2*np.pi*70*dt+.2)
cleaned,diagnostics=Cleaner(reserve_samples=signal.size).clean(
    signal,fs,analysis_window_width_s=1.0,min_tpeak_diff_s=.04,
    cutoff_window_width_s=.2,max_event_width_s=.06,run_count=2)
print(cleaned.shape,diagnostics["attempted_events"],diagnostics["accepted_events"])
