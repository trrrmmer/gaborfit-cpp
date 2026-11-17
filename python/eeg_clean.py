"""ctypes binding for one-call native EEG muscle-artifact cleaning."""
from __future__ import annotations
import ctypes as ct
from pathlib import Path
import platform
import numpy as np

class _Options(ct.Structure):
    _fields_=[("abi_version",ct.c_uint32),("struct_size",ct.c_uint32),
      ("median_filter_duration_s",ct.c_double),("analysis_window_width_s",ct.c_double),
      ("minimum_peak_distance_s",ct.c_double),("fitting_window_width_s",ct.c_double),
      ("maximum_event_width_s",ct.c_double),("pass_count",ct.c_int32),
      ("retain_stage_signals",ct.c_int32)]
class _Result(ct.Structure):
    _fields_=[("abi_version",ct.c_uint32),("struct_size",ct.c_uint32),("status",ct.c_int32),
      ("sample_count",ct.c_size_t),("pass_count",ct.c_size_t),("attempted_events",ct.c_size_t),
      ("accepted_events",ct.c_size_t),("failed_events",ct.c_size_t),("diagnostic",ct.c_char*256)]
class _Event(ct.Structure):
    _fields_=[("pass_index",ct.c_size_t),("peak_index",ct.c_size_t),("window_start",ct.c_int64),("window_end",ct.c_int64),
      ("initial",ct.c_double*5),("lower",ct.c_double*5),("upper",ct.c_double*5),("parameters",ct.c_double*5),
      ("solver_status",ct.c_int32),("rejection",ct.c_int32),("accepted",ct.c_int32),
      ("iterations",ct.c_int32),("evaluations",ct.c_int32),("attempts",ct.c_int32),
      ("qc_triggered",ct.c_int32),("rescue_accepted",ct.c_int32),("bound_active",ct.c_int32),
      ("objective",ct.c_double),("explained_variance",ct.c_double)]
class _Pass(ct.Structure):
    _fields_=[("pass_index",ct.c_size_t),("threshold_peak_count",ct.c_size_t),("duration_valid_peak_count",ct.c_size_t),
      ("selected_peak_count",ct.c_size_t),("attempted_event_count",ct.c_size_t),("accepted_event_count",ct.c_size_t)]

class Cleaner:
    def __init__(self, library=None, reserve_samples=0):
        system=platform.system();suffix={"Windows":".dll","Darwin":".dylib"}.get(system,".so");prefix="" if system=="Windows" else "lib"
        root=Path(__file__).resolve().parents[1];build=root/"build"
        default=build/(prefix+"gaborfit_eeg"+suffix)
        if system=="Windows":default=build/"Release"/("gaborfit_eeg"+suffix)
        if not default.exists():
            legacy=root/"build-eeg";default=(legacy/"Release"/("gaborfit_eeg"+suffix) if system=="Windows" else legacy/(prefix+"gaborfit_eeg"+suffix))
        self.lib=ct.CDLL(str(library or default));self.lib.gaborfit_eeg_abi_version.restype=ct.c_uint32
        if self.lib.gaborfit_eeg_abi_version()!=1:raise RuntimeError("unsupported EEG cleaning ABI")
        self.lib.gaborfit_eeg_context_create.argtypes=[ct.c_size_t];self.lib.gaborfit_eeg_context_create.restype=ct.c_void_p
        self.lib.gaborfit_eeg_context_destroy.argtypes=[ct.c_void_p]
        self.lib.gaborfit_eeg_options_defaults.argtypes=[ct.POINTER(_Options)]
        self.lib.gaborfit_eeg_clean.argtypes=[ct.c_void_p,ct.POINTER(ct.c_double),ct.c_size_t,ct.c_double,ct.POINTER(_Options),ct.POINTER(ct.c_double),ct.POINTER(_Result)]
        self.lib.gaborfit_eeg_clean.restype=ct.c_int32
        self.lib.gaborfit_eeg_last_event_count.argtypes=[ct.c_void_p];self.lib.gaborfit_eeg_last_event_count.restype=ct.c_size_t
        self.lib.gaborfit_eeg_last_event.argtypes=[ct.c_void_p,ct.c_size_t,ct.POINTER(_Event)];self.lib.gaborfit_eeg_last_event.restype=ct.c_int32
        self.lib.gaborfit_eeg_last_pass_count.argtypes=[ct.c_void_p];self.lib.gaborfit_eeg_last_pass_count.restype=ct.c_size_t
        self.lib.gaborfit_eeg_last_pass.argtypes=[ct.c_void_p,ct.c_size_t,ct.POINTER(_Pass)];self.lib.gaborfit_eeg_last_pass.restype=ct.c_int32
        self.context=self.lib.gaborfit_eeg_context_create(reserve_samples)
        if not self.context:raise MemoryError("native context allocation failed")
    def close(self):
        if self.context:self.lib.gaborfit_eeg_context_destroy(self.context);self.context=None
    def __del__(self):
        try:self.close()
        except Exception:pass
    def clean(self,signal,sampling_frequency_hz,**settings):
        source=np.ascontiguousarray(signal,dtype=np.float64)
        if source.ndim!=1:raise ValueError("signal must be one-dimensional")
        options=_Options();self.lib.gaborfit_eeg_options_defaults(ct.byref(options))
        aliases={"min_tpeak_diff_s":"minimum_peak_distance_s","cutoff_window_width_s":"fitting_window_width_s",
                 "max_event_width_s":"maximum_event_width_s","run_count":"pass_count"}
        for key,value in settings.items():setattr(options,aliases.get(key,key),value)
        cleaned=np.empty_like(source);result=_Result()
        status=self.lib.gaborfit_eeg_clean(self.context,source.ctypes.data_as(ct.POINTER(ct.c_double)),source.size,float(sampling_frequency_hz),ct.byref(options),cleaned.ctypes.data_as(ct.POINTER(ct.c_double)),ct.byref(result))
        if status<0:raise RuntimeError(bytes(result.diagnostic).split(b"\0",1)[0].decode())
        events=[]
        for index in range(self.lib.gaborfit_eeg_last_event_count(self.context)):
            e=_Event();assert self.lib.gaborfit_eeg_last_event(self.context,index,ct.byref(e))==0
            events.append({"pass":e.pass_index,"peak_index":e.peak_index,"window_start":e.window_start,"window_end":e.window_end,
              "initial":np.array(e.initial),"lower":np.array(e.lower),"upper":np.array(e.upper),"parameters":np.array(e.parameters),
              "solver_status":e.solver_status,"rejection":e.rejection,"accepted":bool(e.accepted),"objective":e.objective,
              "explained_variance":e.explained_variance,"iterations":e.iterations,"evaluations":e.evaluations,
              "attempts":e.attempts,"qc_triggered":bool(e.qc_triggered),"rescue_accepted":bool(e.rescue_accepted),"bound_active":bool(e.bound_active)})
        passes=[]
        for index in range(self.lib.gaborfit_eeg_last_pass_count(self.context)):
            p=_Pass();assert self.lib.gaborfit_eeg_last_pass(self.context,index,ct.byref(p))==0
            passes.append({name:getattr(p,name) for name,_ in p._fields_})
        return cleaned,{"status":result.status,"pass_count":result.pass_count,"attempted_events":result.attempted_events,
          "accepted_events":result.accepted_events,"failed_events":result.failed_events,"events":events,"passes":passes}
