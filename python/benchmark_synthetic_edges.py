from __future__ import annotations
import csv,time
from pathlib import Path
import numpy as np
from gaborfit import Fitter

ROOT=Path(__file__).resolve().parents[1];fitter=Fitter(reserve_samples=801);rows=[]
cases=[]
for amplitude in (1e-3,1.0,1e3):
  cases.append(("amplitude_scale",1000,201,[amplitude,90,.2,.012,.1],[amplitude*.9,92,.1,.013,.1]))
for fs,n in ((250,51),(1000,201),(4000,801)):
  duration=(n-1)/fs
  cases.append(("near_bounds",fs,n,[2,fs*.47,3.0,1.1/fs,duration*.205],[1.8,fs*.45,2.8,1.5/fs,duration*.21]))
  cases.append(("above_nyquist",fs,n,[2,fs*.57,.3,duration/10,duration*.5],[1.8,fs*.56,.2,duration/9,duration*.5]))
for label,fs,n,truth,initial in cases:
  t=np.arange(n)/fs;truth=np.asarray(truth,float);initial=np.asarray(initial,float)
  y=truth[0]*np.exp(-.5*((t-truth[4])/truth[3])**2)*np.cos(2*np.pi*truth[1]*(t-truth[4])+truth[2])
  duration=t[-1];lower=[truth[0]*.5 if truth[0]>0 else truth[0]*1.5,1,-np.pi,1/fs,duration*.2]
  upper=[truth[0]*1.5 if truth[0]>0 else truth[0]*.5,fs*.8,np.pi,duration*.48,duration*.8]
  began=time.perf_counter_ns();result=fitter.fit(y,initial,lower,upper,sampling_frequency_hz=fs);runtime=(time.perf_counter_ns()-began)*1e-9;p=result["parameters"]
  rows.append(dict(case_type=label,fs_hz=fs,samples=n,truth_amplitude=truth[0],truth_frequency_hz=truth[1],truth_sigma_s=truth[3],status=result["status"],objective=result["objective"],fitted_amplitude=p[0],fitted_frequency_hz=p[1],fitted_sigma_s=p[3],frequency_error_hz=p[1]-truth[1],qc_triggered=result["qc_triggered"],rescue_accepted=result["rescue_accepted"],runtime_s=runtime,bounds_valid=all(lo<=v<=hi for lo,v,hi in zip(lower,p,upper))))
path=ROOT/"results"/"synthetic_edge_cases.csv"
with path.open("w",newline="") as f:w=csv.DictWriter(f,fieldnames=rows[0]);w.writeheader();w.writerows(rows)
assert all(r["bounds_valid"] and r["status"] in (0,1) and np.isfinite(r["objective"]) for r in rows)
print("edge cases",len(rows),"valid",sum(r["bounds_valid"] for r in rows),"converged",sum(r["status"]==0 for r in rows),path)

