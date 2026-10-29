"""TU1.5 party-A countdown leaves; transport/publication is native service policy.

19A78E: initial10, repeated leader Start reduces by1 only above3.
6E360: one decrement after1000ms, restart the clock on each publication;
never catch up several seconds after a stalled pump. 5DF30 clears mode1
and its requester when a lower value is published. 6E410 consumes zero.
"""
import ctypes
from functools import lru_cache
from .protocol import integer, require
from .security_policy import library
from .native_party_countdowns import Countdown as Projection, Snapshot
from .native_matchmaking import value as native_value


@lru_cache(maxsize=1)
def native():
    try:
        lib=library()
        for name,arguments in (
            ("h2_party_countdown_press",[ctypes.c_int32]),
            ("h2_party_countdown_tick",[ctypes.c_int32,ctypes.c_uint32])):
            require(hasattr(lib,name),"Original native party countdown kernel is not installed",503)
            function=getattr(lib,name);function.argtypes=arguments;function.restype=ctypes.c_int32
        return lib
    except (OSError,AttributeError):
        require(False,"Original native party countdown kernel is unavailable",503)


def press(seconds):
    value=-1 if seconds is None else integer(seconds,0,0x7FFFFFFF)
    return native().h2_party_countdown_press(value)


class Countdown(Projection):
    """Numerical fixture carrier; never admitted into the live registry."""
    def __init__(self, serial, seconds, clock_ms, playlist, revision, party_version,
                 mode=0, complete=False, requester=None):
        row = native_value('party_countdown_sample', value=dict(serial=serial, seconds=seconds,
            clock_ms=clock_ms, playlist=playlist, revision=revision, party_version=party_version,
            mode=mode, complete=complete, requester=requester))
        object.__setattr__(self, '_snapshot', Snapshot(**row))

    def update(self, now_ms):
        row = {name: getattr(self, name) for name in Snapshot.__dataclass_fields__}
        value = native_value('party_countdown_sample_tick', value=row, clock_ms=now_ms)
        object.__setattr__(self, '_snapshot', Snapshot(**value))
