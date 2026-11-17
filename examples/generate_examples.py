"""Generate the documented synthetic examples and figures using the real API."""
from pathlib import Path
import csv,sys
import numpy as np
import matplotlib.pyplot as plt
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"python"))
from eeg_clean import Cleaner

out=Path(__file__).resolve().parent/"generated";out.mkdir(exist_ok=True)
rng=np.random.default_rng(20261117)

def make(fs,seconds,events):
    t=np.arange(int(round(fs*seconds)))/fs
    y=.025*np.sin(2*np.pi*9*t)+.006*rng.standard_normal(t.size)
    for center,amplitude,frequency,width in events:
        dt=t-center;y+=amplitude*np.exp(-.5*(dt/width)**2)*np.cos(2*np.pi*frequency*dt+.2)
    return t,y

cases=[
 ("single_isolated",1000,3,[(.45,.25,65,.007),(.9,.22,72,.008),(1.5,3.5,75,.008),(2.1,.2,68,.007),(2.55,.25,70,.008)],1),
 ("multiple_separated",1000,4,[(.5,.25,70,.008),(1.,2.5,72,.008),(1.7,3.,80,.007),(2.5,2.2,65,.009),(3.2,.2,75,.008),(3.6,.25,68,.008)],1),
 ("closely_spaced",1000,3,[(.5,.2,70,.008),(1.3,3.,75,.008),(1.345,2.4,82,.007),(2.2,.2,70,.008),(2.6,.25,65,.008)],1),
 ("dense_activity",1000,4,[(.35+i*.16,1.2+.5*(i%3),60+5*(i%5),.007+.001*(i%2)) for i in range(20)],2),
 ("sampling_500_hz",500,3,[(.4,.2,55,.01),(.9,2.5,65,.009),(1.5,3.,72,.008),(2.2,.2,60,.01),(2.6,.2,65,.009)],1),
 ("three_passes",1000,4,[(.4+i*.22,.25 if i%4 else 3.2,65+4*(i%4),.008) for i in range(16)],3),
]
cleaner=Cleaner(reserve_samples=5000);rows=[];summaries=[]
for name,fs,seconds,events,passes in cases:
    time,signal=make(fs,seconds,events)
    settings=dict(analysis_window_width_s=1.,min_tpeak_diff_s=.04,
                  cutoff_window_width_s=.2,max_event_width_s=.06,run_count=passes)
    cleaned,info=cleaner.clean(signal,fs,**settings)
    summaries.append((name,fs,passes,info["attempted_events"],info["accepted_events"],info["failed_events"]))
    accepted=np.array([e["peak_index"] for e in info["events"] if e["accepted"]],dtype=int)
    fitted=signal-cleaned
    fig,axes=plt.subplots(3,1,figsize=(10,7),sharex=True,constrained_layout=True)
    axes[0].plot(time,signal,lw=.7,color='.3');
    if accepted.size:axes[0].scatter(time[accepted],signal[accepted],s=15,marker='x',color='crimson',label='accepted event');axes[0].legend()
    axes[0].set_ylabel('Original')
    axes[1].plot(time,fitted,lw=.7,color='darkorange');axes[1].set_ylabel('Fitted total')
    axes[2].plot(time,cleaned,lw=.7,color='navy');axes[2].set(ylabel='Cleaned',xlabel='Time (s)')
    fig.suptitle(f'{name}: fs={fs} Hz, passes={passes}, accepted={info["accepted_events"]}')
    fig.savefig(out/f'{name}.png',dpi=180);plt.close(fig)
    for t,y,c,f in zip(time,signal,cleaned,fitted):rows.append((name,fs,passes,t,y,f,c))
with (out/'synthetic_examples.csv').open('w',newline='') as handle:
    writer=csv.writer(handle,lineterminator='\n');writer.writerow(['case','sampling_rate_hz','pass_count','time_s','original','fitted_total','cleaned']);writer.writerows(rows)
with (out/'example_summary.csv').open('w',newline='') as handle:
    writer=csv.writer(handle,lineterminator='\n');writer.writerow(['case','sampling_rate_hz','pass_count','attempted_events','accepted_events','failed_events']);writer.writerows(summaries)
for summary in summaries:print(*summary)
print(f'generated {len(cases)} examples in {out}')
