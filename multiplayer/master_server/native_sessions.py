"""Immutable projections of Rust session, request and failure ownership.

The weak cache preserves the identity of a projection held by socket/room IO.
It contains no independently admitted sessions; every lookup reads Rust.
"""
from collections.abc import Mapping
from dataclasses import dataclass
import weakref

from .couch_players import LocalPlayer
from .protocol import require


@dataclass(frozen=True)
class _Snapshot:
    key: str
    generation: int
    player: str
    screen_name: str
    maps: frozenset[str]
    compatibility: str
    expires: float
    seen: float
    language: int
    local_players: tuple[LocalPlayer, ...]
    appearance: bytes | None


class Session:
    __slots__ = ('_snapshot', '__weakref__')

    def __setattr__(self, name, value):
        raise AttributeError('Session state is owned by Rust')

    def seats(self):
        return self.local_players


for _field in _Snapshot.__dataclass_fields__:
    setattr(Session, _field, property(lambda self, name=_field: getattr(self._snapshot, name)))


class Sessions(Mapping):
    def __init__(self, api):
        self.api, self._views = api, weakref.WeakValueDictionary()

    def project(self, row):
        if row is None:
            return None
        generation = row['generation']
        result = self._views.get(generation)
        if result is None:
            result = Session()
            self._views[generation] = result
        appearance = lambda raw: bytes.fromhex(raw) if raw is not None else None
        snapshot = _Snapshot(**dict(row, maps=frozenset(row['maps']),
            appearance=appearance(row['appearance']), local_players=tuple(
                LocalPlayer(p['player'], p['screen_name'], p['guest'], appearance(p['appearance']))
                for p in row['local_players'])))
        object.__setattr__(result, '_snapshot', snapshot)
        return result

    def generation(self, generation):
        return self.project(self.api.core._call('session_generation', generation=generation))

    def __iter__(self):
        return iter(self.api.core._call('session_keys') if self.api.core else ())

    def __len__(self):
        return len(tuple(iter(self)))

    def __getitem__(self, key):
        row = self.api.core._call('session_get', key=key) if self.api.core else None
        if row is None:
            raise KeyError(key)
        return self.project(row)

    def pop(self, key, *default):
        require(len(default) <= 1, 'Invalid session removal')
        result = self.get(key)
        if result is not None:
            self.api.remove_session(key)
            return result
        if default:
            return default[0]
        raise KeyError(key)

    def clear(self):
        for key in tuple(self):
            self.api.core._call('session_revoke', key=key)
        self.api.session_events()


class Requests(Mapping):
    def __init__(self, api): self.api = api

    def _rows(self):
        return self.api.core._call('session_request_items') if self.api.core else ()

    def __iter__(self): return iter((p, r) for p, r, _ in self._rows())
    def __len__(self): return len(self._rows())

    def __getitem__(self, key):
        for player, request, ticket in self._rows():
            if (player, request) == key:
                return ticket
        raise KeyError(key)


class Failures(Mapping):
    def __init__(self, api): self.api = api
    def __iter__(self): return iter(self.api.core._call('session_failure_keys') if self.api.core else ())
    def __len__(self): return len(tuple(iter(self)))

    def _project(self, key, row):
        if row is not None:
            session = self.api.sessions.generation(row['generation'])
            if session is not None:
                return session, row['code']
        raise KeyError(key)

    def __getitem__(self, key):
        row = self.api.core._call('session_failure_get', player=key) if self.api.core else None
        return self._project(key, row)

    def __setitem__(self, key, item):
        session, code = item
        require(type(session) is Session and key == session.player, 'Admitted session required')
        self.api.core._call('session_failure_set', generation=session.generation, code=code)

    def pop(self, key, *default):
        require(len(default) <= 1, 'Invalid failure removal')
        row = self.api.core._call('session_failure_pop', player=key) if self.api.core else None
        try:
            return self._project(key, row)
        except KeyError:
            if default:
                return default[0]
            raise

    def clear(self):
        for key in tuple(self):
            self.pop(key)
