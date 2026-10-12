"""Recovered client search/gather clocks and completion arithmetic.

Original owners:0x70190,70D60,70EA0,90F70,71730,1931D0. This module does
not emulate Live task allocation, NAT discovery or platform reservations.
See ../docs/halo2-gathering-session-evidence.md.
"""
from dataclasses import dataclass, field
from functools import cache
import json
from pathlib import Path

I32_MAX = 0x7fffffff


def signed(value, bits=32):
    return (value+(1 << (bits-1))) % (1 << bits)-(1 << (bits-1))


def elapsed(now, start):
    return signed(now-start)


def jitter(seed, search_spread=60000, gather_spread=60000):
    """Complete0x70190 LCG and signed-low16 range casts, in original order."""
    values = []
    for spread in (search_spread, gather_spread):
        seed = (seed*1664525+1013904223) & 0xffffffff
        product = ((seed >> 16)*signed(spread,16)) & 0xffffffff
        values.append(signed(product >> 16,16))
    return seed, *values


def role(now, search_start, gather_start, capacity):
    # Original default restricted-NAT flag is false. The native relay does not
    # invent an Xbox NAT type or enable that optional policy.
    return ("search" if not search_start or elapsed(now,search_start) >= elapsed(now,gather_start)
            or capacity == -1 else "gather")


@dataclass
class SearchProgress:
    maximum: int
    ranked: bool
    radius: int = 0  # -1 is the original unbounded casual retry.
    total: int = 0
    active: bool = True

    def complete(self, count, *, query_pending=False, full_pending=False,
                 brief_pending=False, selected=False):
        """90F70: returns whether another query must be submitted."""
        if not self.active or query_pending or full_pending or brief_pending or selected:
            return False
        if type(count) is not int or not 0 <= count <= 50:
            raise ValueError("Invalid trusted query result count")
        self.total = signed(self.total+count)
        changed = False
        if self.total < 150:
            if self.radius != -1:
                if self.radius < self.maximum:
                    self.radius += 1
                    changed = True
                elif self.radius == self.maximum and not self.ranked:
                    self.radius, changed = -1, True
            if count == 50 or changed:
                return True
        self.active = False
        return False


@dataclass
class SeenSessions:
    # Matched0x90880 reads0x4CE1F8:150. The decompile's named1F8
    # configuration field points elsewhere and would incorrectly give25.
    values: set[str] = field(default_factory=set)

    def reject(self, identity):
        if len(self.values) < 150:
            self.values.add(identity)

    def clear(self):
        self.values.clear()

    def __contains__(self, identity):
        return identity in self.values

    def __len__(self):
        return len(self.values)


@dataclass(frozen=True)
class GatheringConfiguration:
    countdown_ms: tuple[int, ...]
    timeout_ms: int
    provenance: str

    def __post_init__(self):
        if (type(self.countdown_ms) is not tuple or len(self.countdown_ms) != 16 or
                any(type(n) is not int or not 0 <= n <= I32_MAX for n in self.countdown_ms) or
                type(self.timeout_ms) is not int or not 10000 <= self.timeout_ms <= 3600000 or
                type(self.provenance) is not str or not 1 <= len(self.provenance) <= 512):
            raise ValueError("Explicit gathering countdown, timeout and provenance required")

    def delay(self, players, minimum, maximum):
        if not minimum <= players <= maximum:
            return I32_MAX
        return 0 if players == maximum else self.countdown_ms[players-1]


@dataclass
class GatheringClock:
    started: int
    growth: int
    peak: int = 0

    def observe(self, now, players):
        if players > self.peak:
            self.peak, self.growth = players, now
        return elapsed(now,self.growth)

    def expired(self, now, configuration, offset):
        return elapsed(now,self.growth) >= signed(configuration.timeout_ms+offset)


@cache
def retail_configuration(index):
    if type(index) is not int or not 0 <= index <= 6:
        raise ValueError("Unknown retail launch hopper")
    data = json.loads(Path(__file__).with_name('retail_gathering.json').read_text())
    row = data['hoppers'][index]
    return row['minimum'],row['maximum'],GatheringConfiguration(tuple(row['countdown_ms']),row['timeout_ms'],
        'Retail launch2004 '+row['name']+' countdown/timeout;0x193C70/1931D0')
