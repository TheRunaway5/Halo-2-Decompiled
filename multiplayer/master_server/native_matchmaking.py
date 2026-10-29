"""Temporary typed IO to the authoritative Rust matchmaking owner.

Only immutable snapshots cross this private ABI. Queue maps, reservations,
readiness, catalog validation, policy decisions and RNG remain inside Rust.
"""
from collections.abc import Mapping, MutableMapping
import ctypes
from dataclasses import fields, is_dataclass
from fractions import Fraction
from functools import cache, cached_property
import json
import random
import time
import weakref

from .protocol import Problem, require
from .security_policy import library

REQUEST_LIMIT = 8 * 1024 * 1024
RESPONSE_LIMIT = 2 * 1024 * 1024


@cache
def native():
    lib = library()
    pointer, size, owner = ctypes.c_void_p, ctypes.c_size_t, ctypes.c_uint64
    signatures = {
        'h2_matchmaking_owner_new': ([pointer, size], owner),
        'h2_matchmaking_owner_drop': ([owner], None),
        'h2_matchmaking_object_open': ([pointer, size, ctypes.POINTER(size)], pointer),
        'h2_matchmaking_owner_request': ([owner, pointer, size, ctypes.POINTER(size)], pointer),
        'h2_matchmaking_value': ([pointer, size, ctypes.POINTER(size)], pointer),
        'h2_matchmaking_buffer_free': ([pointer, size], None),
        'h2_matchmaking_buffers_live': ([], size),
        'h2_matchmaking_test_inputs_enabled': ([], ctypes.c_bool),
    }
    for name, (arguments, result) in signatures.items():
        require(hasattr(lib, name), 'Rust matchmaking library requires an update', 503)
        function = getattr(lib, name)
        function.argtypes, function.restype = arguments, result
    return lib


def pack(value):
    if is_dataclass(value) and not isinstance(value, type):
        return {field.name: pack(getattr(value, field.name)) for field in fields(value)}
    if type(value) is Fraction:
        return [value.numerator, value.denominator]
    if type(value) is frozenset:
        return sorted(value)
    if type(value) in (tuple, list):
        return [pack(item) for item in value]
    if type(value) is dict:
        return {key: pack(item) for key, item in value.items()}
    return value


def _carrier(value):
    try:
        raw = json.dumps(pack(value), separators=(',', ':'), allow_nan=False).encode()
    except (TypeError, ValueError, OverflowError):
        raise Problem(400, 'Invalid typed matchmaking carrier') from None
    require(len(raw) <= REQUEST_LIMIT, 'Oversize matchmaking carrier')
    return ctypes.create_string_buffer(raw), len(raw)


def _response(function, *prefix, request):
    source, length = _carrier(request)
    count = ctypes.c_size_t()
    lib = native()
    pointer = function(*prefix, source, length, ctypes.byref(count))
    require(pointer, 'Rust matchmaking owner unavailable', 503)
    try:
        require(0 < count.value <= RESPONSE_LIMIT, 'Invalid Rust matchmaking response', 503)
        result = json.loads(ctypes.string_at(pointer, count.value))
    finally:
        lib.h2_matchmaking_buffer_free(pointer, count.value)
    require(result['ok'], result.get('error', 'Rust matchmaking rejected operands'), result.get('status', 503))
    return result['value']


def value(op, **values):
    return _response(native().h2_matchmaking_value, request=dict(op=op, **values))


def _construct(cls, values):
    """Project an already validated Rust snapshot without rerunning policy."""
    result = object.__new__(cls)
    for name, item in values.items():
        object.__setattr__(result, name, item)
    return result


def project(kind, data):
    if data is None:
        return None
    from . import matchmaking as model
    if kind == 'catalog':
        from .matchmaking_http import CatalogEntry
        return _construct(CatalogEntry, dict(data, policy=project('playlists', data['policy'])))
    if kind == 'ready':
        return frozenset(data)
    if kind == 'connections':
        return tuple(data)
    if kind == 'stages':
        return _construct(model.SearchStage, dict(data, rank_fraction=Fraction(*data['rank_fraction'])))
    if kind == 'games':
        return _construct(model.Game, data)
    if kind == 'players':
        return _construct(model.Player, dict(data, maps=frozenset(data['maps'])))
    if kind == 'tickets':
        return _construct(model.Ticket, dict(data, players=tuple(project('players', p) for p in data['players'])))
    if kind == 'proposals':
        return _construct(model.Proposal, dict(data, tickets=tuple(data['tickets']),
            teams=tuple(tuple(team) for team in data['teams']), game=project('games', data['game'])))
    if kind == 'playlists':
        from .search_lifecycle import GatheringConfiguration
        return _construct(model.Playlist, dict(data,
            games=tuple(project('games', g) for g in data['games']),
            stages=tuple(project('stages', s) for s in data['stages']),
            party_balance=None if data['party_balance'] is None else _construct(model.PartyBalance, data['party_balance']),
            clan_match=None if data['clan_match'] is None else _construct(model.ClanMatch, data['clan_match']),
            gathering=None if data['gathering'] is None else _construct(GatheringConfiguration,
                dict(data['gathering'], countdown_ms=tuple(data['gathering']['countdown_ms'])))))
    return data


class Defaults(Mapping):
    """Lazy immutable Rust metadata, so importing model types needs no library."""
    def __init__(self, name, cls):
        self.name, self.cls = name, cls

    @cached_property
    def _items(self):
        return {key: _construct(self.cls, row) for key, row in value('defaults')[self.name].items()}

    def __iter__(self): return iter(self._items)
    def __len__(self): return len(self._items)
    def __getitem__(self, key): return self._items[key]


class _Map(Mapping):
    def __init__(self, owner, name):
        self.owner, self.name = owner, name

    def __iter__(self):
        offset = 0
        while True:
            keys = self.owner._call('keys', map=self.name, offset=offset, limit=1000)
            yield from (tuple(key) if self.name == 'connections' else key for key in keys)
            if len(keys) < 1000:
                return
            offset += len(keys)

    def __len__(self):
        return self.owner._call('len', map=self.name)

    def __getitem__(self, key):
        try:
            return project(self.name, self.owner._call('get', map=self.name, key=key))
        except Problem as error:
            if error.status == 404:
                raise KeyError(key) from None
            raise


class _Catalog(_Map, MutableMapping):
    def __setitem__(self, key, policy):
        from .matchmaking import Playlist
        require(type(policy) is Playlist, 'Typed playlist required')
        self.owner._call('policy_put', key=key, value=policy)

    def __delitem__(self, key):
        raise TypeError('Replace the complete admitted catalog to remove a playlist')


class _Connections(_Map):
    def pop(self, key, *default):
        require(len(default) <= 1, 'Invalid connection removal')
        result = self.owner._call('connection_delete', key=key)
        if result is not None:
            return tuple(result)
        if default:
            return default[0]
        raise KeyError(key)


class _Hoppers:
    def __init__(self, owner): self.owner = owner

    @property
    def seed(self): return self.owner._setting('draw_seed')
    @seed.setter
    def seed(self, seed): self.owner._set_setting('draw_seed', seed)

    def choose(self, games, players):
        games, players = tuple(games), tuple(players)
        index = self.owner._call('draw', games=games, players=players)
        return None if index is None else games[index]


def _setting_property(name):
    return property(lambda self: self._setting(name), lambda self, v: self._set_setting(name, v))


class Coordinator:
    """Trusted IO facade. Rust is the sole queue and proposal authority."""
    def __init__(self, playlists, *, clock=time.monotonic, rng=None, lease_seconds=90,
                 proposal_seconds=30, qos_seconds=30, capacity=1000, search_budget=10000, catalog=None):
        self.clock, self.rng = clock, rng or random.SystemRandom()
        # Entropy ingestion only. All subsequent draws use separate Rust-owned
        # original hopper and explicitly modern scheduler/host streams.
        setup = dict(playlists=tuple(playlists), lease=lease_seconds, proposal_lease=proposal_seconds,
            qos_lease=qos_seconds, capacity=capacity, search_budget=search_budget,
            draw_seed=self.rng.getrandbits(32), native_seed=self.rng.getrandbits(32))
        if catalog is not None:
            setup['catalog'] = tuple(catalog)
        value('validate_owner', value=setup)
        source, length = _carrier(setup)
        lib = native()
        self._handle = lib.h2_matchmaking_owner_new(source, length)
        require(self._handle, 'Rust matchmaking allocation failed', 503)
        self._finalizer = weakref.finalize(self, lib.h2_matchmaking_owner_drop, self._handle)
        self._playlists = _Catalog(self, 'playlists')
        self.tickets, self.proposals, self.ready = (_Map(self, name) for name in ('tickets', 'proposals', 'ready'))
        self.connections, self.next_anchor = _Connections(self, 'connections'), _Map(self, 'next_anchor')
        self.hoppers = _Hoppers(self)

    def close(self):
        self._finalizer()
        self._handle = 0

    def _call(self, op, **values):
        require(self._handle, 'Rust matchmaking owner retired', 503)
        return _response(native().h2_matchmaking_owner_request, self._handle,
                         request=dict(op=op, now=self.clock(), **values))

    @property
    def playlists(self): return self._playlists
    @playlists.setter
    def playlists(self, policies):
        self._call('policies_set', values=tuple(policies.values()))

    def _setting(self, name): return self._call('setting_get', name=name)
    def _set_setting(self, name, item): self._call('setting_set', name=name, value=item)
    lease = _setting_property('lease')
    proposal_lease = _setting_property('proposal_lease')
    qos_lease = _setting_property('qos_lease')
    capacity = _setting_property('capacity')
    search_budget = _setting_property('search_budget')

    @property
    def catalog(self):
        return tuple(project('catalog', row) for row in self._call('catalog_get', id=None))
    @catalog.setter
    def catalog(self, entries):
        self._call('catalog_set', entries=tuple(entries))
    def entry(self, playlist):
        return project('catalog', self._call('catalog_get', id=playlist))
    def eligible(self, playlist, compatibility, connections):
        return self._call('catalog_eligible', id=playlist, compatibility=compatibility, connections=connections)
    def common_clan(self, playlist, seats):
        return self._call('catalog_clan_group', id=playlist, seats=seats)

    def now(self): return self._call('now')
    def cleanup(self): self._call('cleanup')
    def enqueue(self, party_id, playlist_id, compatibility, players, *, clan=None, clan_rank=None, leader=None, language=0):
        from .matchmaking import Player
        require(type(players) is tuple and all(type(p) is Player for p in players), 'Trusted player tuple required')
        return project('tickets', self._call('enqueue', party_id=party_id, playlist_id=playlist_id,
            compatibility=compatibility, players=players, clan=clan, clan_rank=clan_rank, leader=leader, language=language))

    def heartbeat(self, ticket_id): self._call('heartbeat', id=ticket_id)
    def cancel(self, ticket_id): self._call('cancel', id=ticket_id)
    def connection(self, host, player, score): self._call('connection', host=host, player=player, score=score)
    def _stage(self, policy, ticket):
        return project('stages', self._call('stage', playlist=policy.id, ticket=ticket.id))
    def _radius(self, policy, rank, *, raw=False): return value('radius', policy=policy, rank=rank, raw=raw)
    def _compatible(self, policy, a, b): return self._call('compatible', playlist=policy.id, a=a.id, b=b.id)
    def _teams(self, policy, group):
        result = self._call('teams', playlist=policy.id, tickets=tuple(t.id for t in group))
        return None if result is None else tuple(tuple(team) for team in result)
    def _host(self, policy, group, preferred=None, *, required=None, host_quality=None):
        return self._call('host', playlist=policy.id, tickets=tuple(t.id for t in group),
                          preferred=preferred, required=required, quality=host_quality)
    def advance(self, playlist_id, *, allow_pending_connectivity=False):
        self.cleanup()
        return project('proposals', self._call('advance', playlist=playlist_id, pending=allow_pending_connectivity))
    def form_proposal(self, playlist_id, tickets, game=None, gathering_host=None):
        return project('proposals', self._call('form', playlist=playlist_id, tickets=tickets,
                                              game=game, gathering_host=gathering_host))
    def fits(self, policy, group, ticket, game=None):
        return self._call('fits', playlist=policy.id, tickets=tuple(t.id for t in group), ticket=ticket.id, game=game)
    def refresh_connections(self, proposal_id, *, optimize_host=False, host_quality=None):
        return project('proposals', self._call('refresh', id=proposal_id, optimize=optimize_host, quality=host_quality))
    def acknowledge(self, proposal_id, player_id, *, optimize_host=False, host_quality=None):
        return project('proposals', self._call('acknowledge', id=proposal_id, player=player_id,
                                              optimize=optimize_host, quality=host_quality))
    def status(self, ticket_id): return self._call('status', id=ticket_id)
    def admission(self, proposal_id): return self._call('admission', id=proposal_id)
