from pathlib import Path
import sys
import numpy as np
sys.path.insert(0, str(Path(__file__).parent))
from gaborfit import Fitter

fs=1000.0; t=np.arange(201)/fs; p=np.array([3.0,80.0,.35,.014,.1])
y=p[0]*np.exp(-.5*((t-p[4])/p[3])**2)*np.cos(2*np.pi*p[1]*(t-p[4])+p[2])
fit=Fitter(reserve_samples=len(y))
r=fit.fit(y,[2.8,82,.2,.016,.1],[.5,5,-np.pi,.002,.07],[5,300,np.pi,.05,.13],sampling_frequency_hz=fs)
assert r["status"] == 0 and r["objective"] < 1e-10, r
fit.close(); print("ctypes smoke test passed",r["objective"])

