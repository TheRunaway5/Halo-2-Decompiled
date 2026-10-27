"""TU1.5 party-A countdown leaves; transport/publication is native service policy.

19A78E: initial10, repeated leader Start reduces by1 only above3.
6E360: one decrement after1000ms, restart the clock on each publication;
never catch up several seconds after a stalled pump. 5DF30 clears mode1
and its requester when a lower value is published. 6E410 consumes zero.
"""
import ctypes
from dataclasses import dataclass
from functools import lru_cache
from .protocol import integer, require
from .security_policy import library


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


@dataclass
class Countdown:
    serial: int
    seconds: int | None
    clock_ms: int
    playlist: str
    revision: str
    party_version: int
    mode: int = 0
    complete: bool = False
    requester: str | None = None

    def update(self, now_ms):
        if self.seconds is None:
            return
        value=native().h2_party_countdown_tick(integer(self.seconds,0,0x7FFFFFFF),(now_ms-self.clock_ms)&0xFFFFFFFF)
        if value != self.seconds:
            self.seconds = value
            self.clock_ms = now_ms&0xFFFFFFFF
            self.mode = 0
            self.requester = None
            self.complete = self.seconds == 0

    def public(self):
        return dict(serial=self.serial, seconds=self.seconds, mode=self.mode, complete=self.complete)
