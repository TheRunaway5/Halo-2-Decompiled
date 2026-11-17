"""Read-only reuse of another factory track's oracle fixtures.

Composed kernels (one track's owner calling another track's natives) need the
callee track's fixture builders. borrow(v, track) loads
tools/native_object_factory_cases_<track>.py as a private module and runs its
register() against a capturing view of the validator module `v`:

- the track's KERNELS, ROUTINES, TRACK_ARGUMENTS and TRACK_COUNTS, and any
  wrapper it assigns to v (cases, metadata, arguments), stay private to the
  capture, so borrowing never registers or wraps anything;
- its global closure declarations (REGISTER_JUMPS, JUMP_TABLES,
  CLOSURE_STOPS, UNREACHED_VIRTUAL_OWNERS), mutated in place, are the shared
  ones, which the borrowed fixtures' original paths need anyway.

Returns (module, capture). The capture's TRACK_ARGUMENTS holds the track's
case builders; module-level names (constants, a FIXTURES dict a track may
publish) come from the private module. Builders close over the capture,
which forwards every other attribute to `v`.

This file is not a track (the validator only globs
native_object_factory_cases_*.py).
"""
import importlib.util
from pathlib import Path

_PRIVATE = ('KERNELS', 'ROUTINES', 'TRACK_ARGUMENTS', 'TRACK_COUNTS')
_BORROWED = {}


class Capture:
    def __init__(self, v):
        object.__setattr__(self, '_v', v)
        for name in _PRIVATE:
            object.__setattr__(self, name, {})

    def __getattr__(self, name):
        return getattr(self._v, name)

    def __setattr__(self, name, value):
        # Wrappers a track installs (v.cases = ..., v.metadata = ...) stay here.
        object.__setattr__(self, name, value)


def borrow(v, track):
    key = (id(v), track)
    if key not in _BORROWED:
        path = Path(__file__).resolve().parent / f'native_object_factory_cases_{track}.py'
        spec = importlib.util.spec_from_file_location(f'borrowed_{track}_fixtures', path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        capture = Capture(v)
        module.register(capture)
        _BORROWED[key] = (module, capture)
    return _BORROWED[key]
