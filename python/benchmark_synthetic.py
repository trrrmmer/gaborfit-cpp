from __future__ import annotations
import csv, math, time
from pathlib import Path
import numpy as np
from gaborfit import Fitter

ROOT=Path(__file__).resolve().parents[1]; OUT=ROOT/"results";OUT.mkdir(exist_ok=True)
rng=np.random.default_rng(20261117); fitter=Fitter(reserve_samples=801); rows=[]
for fs in (250,500,1000,2000,4000):
  for n in (51,101,201,401,801):
    for snr_db in (math.inf,40,20):
      for nonuniform in (False,True):
        t=np.arange(n,dtype=float)/fs
        if nonuniform and n>2: t[1:-1]+=0.05/fs*np.sin(np.arange(1,n-1)*.71)
        duration=t[-1]-t[0]; truth=np.array([2.5,min(80.,fs*.18),.4,max(3/fs,duration/9),duration*.51])
        clean=truth[0]*np.exp(-.5*((t-truth[4])/truth[3])**2)*np.cos(2*np.pi*truth[1]*(t-truth[4])+truth[2])
        noise_rms=0 if math.isinf(snr_db) else np.sqrt(np.mean(clean**2))/10**(snr_db/20)
        y=clean+rng.normal(0,noise_rms,n);weights=np.ones(n);weights[[1,-2]]=0
        lower=[.25,1,-np.pi,1/fs,duration*.2];upper=[5,fs*.49,np.pi,duration*.48,duration*.8]
        initial=[2.2,truth[1]*1.03,.3,truth[3]*1.1,truth[4]*.999]
        kwargs={"times":t,"sampling_frequency_hz":fs} if nonuniform else {"sampling_frequency_hz":fs}
        fitter.fit(y,initial,lower,upper,weights=weights,**kwargs)
        durations=[]
        for _ in range(5):
          began=time.perf_counter_ns();result=fitter.fit(y,initial,lower,upper,weights=weights,**kwargs);durations.append((time.perf_counter_ns()-began)*1e-9)
        p=result["parameters"];wave=p[0]*np.exp(-.5*((t-p[4])/p[3])**2)*np.cos(2*np.pi*p[1]*(t-p[4])+p[2])
        rows.append(dict(fs_hz=fs,samples=n,snr_db=snr_db,nonuniform=nonuniform,status=result["status"],objective=result["objective"],
          waveform_nrmse=float(np.linalg.norm(wave-clean)/max(np.linalg.norm(clean),1e-30)),frequency_error_hz=p[1]-truth[1],sigma_error_s=p[3]-truth[3],
          runtime_p50_s=float(np.quantile(durations,.5)),runtime_p95_s=float(np.quantile(durations,.95)),attempts=result["attempts"],qc_triggered=result["qc_triggered"]))
with (OUT/"synthetic_generalization.csv").open("w",newline="") as f:
  writer=csv.DictWriter(f,fieldnames=rows[0]);writer.writeheader();writer.writerows(rows)
assert len(rows)==150
assert all(r["status"] in (0,1) and np.isfinite(r["objective"]) for r in rows)
assert max(r["waveform_nrmse"] for r in rows)<0.1
print("synthetic cases",len(rows),"converged",sum(r["status"]==0 for r in rows),"output",OUT/"synthetic_generalization.csv")

