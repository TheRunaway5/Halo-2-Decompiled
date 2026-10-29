"""Typed IO and read-only projections of Rust complete-world coordination.

Actual rooms attach to the owner installed from Rust's immutable admission.
Standalone objects use the same Rust state machine; they cannot be installed
into a live room. Digests remain peer claims, not evidence of world import.
"""
from dataclasses import dataclass
import time
from types import MappingProxyType
import weakref

from .native_matchmaking import native, _response
from .protocol import require


@dataclass(frozen=True)
class WorldSeal:
    tick: int
    digest: str


class PlayerHostedContinuity:
    __slots__ = ('_handle', '_finalizer', '_source', '_id', 'clock', '__weakref__')

    def __setattr__(self, name, value):
        if name != 'clock':
            raise AttributeError('Continuity state is owned by Rust')
        object.__setattr__(self, name, value)

    def __init__(self, owners, host, *, clock=time.monotonic, timeout=15):
        lib = native()
        handle = _response(lib.h2_matchmaking_object_open,
                           request={'continuity':dict(owners=tuple(owners), host=host, timeout=timeout)})
        object.__setattr__(self, '_source', None)
        object.__setattr__(self, '_id', None)
        object.__setattr__(self, '_handle', handle)
        object.__setattr__(self, '_finalizer', weakref.finalize(self, lib.h2_matchmaking_owner_drop, handle))
        self.clock = clock

    @classmethod
    def attach(cls, source, room):
        result = object.__new__(cls)
        object.__setattr__(result, '_source', source)
        object.__setattr__(result, '_id', room)
        object.__setattr__(result, '_handle', None)
        object.__setattr__(result, '_finalizer', None)
        result.clock = lambda: source.clock()
        result._call('snapshot')  # An attachment cannot construct or restore state.
        return result

    def _call(self, op, **values):
        if self._source is None:
            require(self._handle, 'Rust continuity owner retired', 503)
            handle, request = self._handle, dict(op=op, now=self.clock(), **values)
        else:
            require(self._source._handle, 'Rust matchmaking owner retired', 503)
            handle = self._source._handle
            request = dict(op='room_continuity_'+op, id=self._id, now=self.clock(), **values)
        return _response(native().h2_matchmaking_owner_request, handle, request=request)

    def close(self):
        if self._finalizer is not None:
            self._finalizer()
            object.__setattr__(self, '_handle', 0)

    def _snapshot(self):
        return self._call('snapshot')

    def _state(self):
        # Packet/expiry readers do not copy the bounded world-proof history.
        return self._call('state')

    @property
    def owners(self): return tuple(self._state()['owners'])
    @property
    def active(self): return frozenset(self._state()['active'])
    @property
    def rebound(self): return frozenset(self._state()['rebound'])
    @property
    def failed_candidates(self): return frozenset(self._state()['failed_candidates'])
    @property
    def quality(self): return MappingProxyType({int(k):v for k,v in self._state()['quality'].items()})
    @property
    def selected(self):
        row = self._snapshot()['selected']
        return None if row is None else WorldSeal(**row)
    @property
    def seals(self): return self._proofs('seals')
    @property
    def proofs(self): return self._proofs('proofs')
    def _proofs(self, field):
        return MappingProxyType({int(k):WorldSeal(**v) for k,v in self._snapshot()[field].items()})

    def observe_quality(self, peer, milliseconds):
        return self._call('quality', peer=peer, milliseconds=milliseconds)

    def seal(self, peer, epoch, tick, digest):
        return WorldSeal(**self._call('seal', peer=peer, epoch=epoch, tick=tick, digest=digest))

    def acknowledge(self, peer, epoch, tick, digest):
        return self._call('acknowledge', peer=peer, epoch=epoch, tick=tick, digest=digest)

    def disconnect(self, peer): return self._call('disconnect', peer=peer)
    def expire(self): return self._call('expire')
    def control(self): return self._call('control')

    def accept(self, peer, epoch, tick, digest):
        return self._call('accept', peer=peer, epoch=epoch, tick=tick, digest=digest)

    def rebind(self, peer, epoch, tick, digest):
        return self._call('rebind', peer=peer, epoch=epoch, tick=tick, digest=digest)


for _field in ('host', 'epoch', 'phase', 'members', 'timeout', 'deadline'):
    setattr(PlayerHostedContinuity, _field, property(lambda self, name=_field: self._state()[name]))
