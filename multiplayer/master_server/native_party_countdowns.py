"""Read-only snapshots of the Rust pregame countdown owner.

The weak cache updates a held projection when clock IO delivers a native tick.
It grants no path to install, mutate, or restore a live countdown.
"""
from collections.abc import Mapping
from dataclasses import dataclass
import weakref


@dataclass(frozen=True)
class Snapshot:
    serial: int
    seconds: int | None
    clock_ms: int
    playlist: str
    revision: str
    party_version: int
    mode: int
    complete: bool
    requester: str | None


class Countdown:
    __slots__ = ('_snapshot', '__weakref__')

    def __setattr__(self, name, value):
        raise AttributeError('Countdown state is owned by Rust')


for _field in Snapshot.__dataclass_fields__:
    setattr(Countdown, _field, property(lambda self, name=_field: getattr(self._snapshot, name)))


class Countdowns(Mapping):
    def __init__(self, api):
        self.api, self._views = api, weakref.WeakValueDictionary()

    def project(self, row):
        if row is None:
            return None
        result = self._views.get(row['serial'])
        if result is None:
            result = Countdown()
            self._views[row['serial']] = result
        object.__setattr__(result, '_snapshot', Snapshot(**row))
        return result

    def refresh(self, rows):
        for row in rows:
            if row['serial'] in self._views:
                self.project(row)

    def __iter__(self):
        core = self.api.directory.matchmaking.core
        return iter(core._call('party_countdown_keys') if core else ())

    def __len__(self):
        return len(tuple(iter(self)))

    def copy(self):
        return dict(self.items())

    def __getitem__(self, key):
        core = self.api.directory.matchmaking.core
        row = core._call('party_countdown_get', party=key) if core else None
        if row is None:
            raise KeyError(key)
        return self.project(row)
