"""Thin ctypes binding for the stable gaborfit C ABI (parameters use SI/radians)."""
from __future__ import annotations
import ctypes as ct
import os
import platform
from pathlib import Path
import numpy as np


class _Request(ct.Structure):
    _fields_ = [("abi_version",ct.c_uint32),("struct_size",ct.c_uint32),("signal", ct.POINTER(ct.c_double)), ("times", ct.POINTER(ct.c_double)),
                ("weights", ct.POINTER(ct.c_double)), ("sample_count", ct.c_size_t),
                ("sampling_frequency_hz", ct.c_double), ("initial", ct.c_double * 5),
                ("lower", ct.c_double * 5), ("upper", ct.c_double * 5),
                ("max_iterations", ct.c_int32), ("fallback_iterations", ct.c_int32),
                ("x_tolerance", ct.c_double), ("f_tolerance", ct.c_double),
                ("gradient_tolerance", ct.c_double), ("initial_damping", ct.c_double),
                ("automatic_initialization", ct.c_int32),
                ("deterministic_fallback", ct.c_int32), ("quality_control", ct.c_int32)]


class _Result(ct.Structure):
    _fields_ = [("abi_version",ct.c_uint32),("struct_size",ct.c_uint32),("status", ct.c_int32), ("parameters", ct.c_double * 5),
                ("objective", ct.c_double), ("residual_rms", ct.c_double),
                ("iterations", ct.c_int32), ("evaluations", ct.c_int32),
                ("attempts", ct.c_int32), ("qc_triggered", ct.c_int32),
                ("rescue_accepted", ct.c_int32), ("bound_active", ct.c_int32),
                ("diagnostic", ct.c_char * 256)]


def _default_library() -> Path:
    suffix = {"Windows": ".dll", "Darwin": ".dylib"}.get(platform.system(), ".so")
    prefix = "" if platform.system() == "Windows" else "lib"
    build = Path(__file__).resolve().parents[1] / "build"
    candidates = [build / f"{prefix}gaborfit{suffix}"]
    if platform.system() == "Windows":
        candidates = [build / configuration / f"gaborfit{suffix}"
                      for configuration in ("Release", "RelWithDebInfo", "Debug")] + candidates
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    return candidates[0]


class Fitter:
    def __init__(self, library: str | os.PathLike | None = None, reserve_samples: int = 0):
        self.library = ct.CDLL(str(library or _default_library()))
        self.library.gaborfit_abi_version.restype = ct.c_uint32
        if self.library.gaborfit_abi_version() != 1:
            raise RuntimeError("unsupported gaborfit ABI")
        self.library.gaborfit_context_create.argtypes = [ct.c_size_t]
        self.library.gaborfit_context_create.restype = ct.c_void_p
        self.library.gaborfit_context_destroy.argtypes = [ct.c_void_p]
        self.library.gaborfit_context_destroy.restype = None
        self.library.gaborfit_request_defaults.argtypes = [ct.POINTER(_Request)]
        self.library.gaborfit_request_defaults.restype = None
        self.library.gaborfit_fit.argtypes = [ct.c_void_p, ct.POINTER(_Request), ct.POINTER(_Result)]
        self.library.gaborfit_fit.restype = ct.c_int32
        self._context = self.library.gaborfit_context_create(reserve_samples)
        if not self._context:
            raise MemoryError("failed to create native fitter")

    def close(self):
        if self._context:
            self.library.gaborfit_context_destroy(self._context); self._context = None

    def __del__(self):
        try: self.close()
        except Exception: pass

    def fit(self, signal, initial, lower, upper, *, sampling_frequency_hz=0.0,
            times=None, weights=None, automatic_initialization=False,
            max_iterations=100, fallback_iterations=200, quality_control=True):
        arrays = [np.ascontiguousarray(signal, dtype=np.float64)]
        if arrays[0].ndim != 1 or arrays[0].size < 5: raise ValueError("signal must be a 1-D array with >=5 samples")
        time_array = None if times is None else np.ascontiguousarray(times, dtype=np.float64)
        weight_array = None if weights is None else np.ascontiguousarray(weights, dtype=np.float64)
        for value, name in ((time_array, "times"), (weight_array, "weights")):
            if value is not None and value.shape != arrays[0].shape: raise ValueError(f"{name} shape mismatch")
        pointer = lambda value: None if value is None else value.ctypes.data_as(ct.POINTER(ct.c_double))
        request = _Request(1,ct.sizeof(_Request),pointer(arrays[0]), pointer(time_array), pointer(weight_array), arrays[0].size,
            sampling_frequency_hz, (ct.c_double * 5)(*initial), (ct.c_double * 5)(*lower),
            (ct.c_double * 5)(*upper), max_iterations, fallback_iterations, 1e-5, 1e-5, 1e-8,
            1e-3, automatic_initialization, True, quality_control)
        result = _Result(); self.library.gaborfit_fit(self._context, ct.byref(request), ct.byref(result))
        return {"status": result.status, "parameters": np.array(result.parameters),
                "objective": result.objective, "residual_rms": result.residual_rms,
                "iterations": result.iterations, "evaluations": result.evaluations,
                "attempts": result.attempts, "qc_triggered": bool(result.qc_triggered),
                "rescue_accepted": bool(result.rescue_accepted),
                "bound_active": bool(result.bound_active),
                "diagnostic": bytes(result.diagnostic).split(b"\0", 1)[0].decode()}

