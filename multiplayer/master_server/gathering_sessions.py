"""Temporary transport IO and immutable views for Rust-owned gatherings.

Rust owns registries, advertisement production, provider order, query caches,
roles, clocks, probe failure/seen transitions, admission and finalization.
The adapter reads requested QoS facts and executes Rust-issued probe requests.
The relay probe and native provider remain explicit modern replacements.
"""
from collections.abc import Mapping
from dataclasses import dataclass
import secrets
import time

from .matchmaking import Game
from .native_matchmaking import native, value, project, _construct
from .protocol import Problem, require
from .search_lifecycle import PartyState
from .session_candidates import Search
from .matchmaking_ratings import (RETAIL_RANK_LIMITS, rank_window,
                                 advertised_minimum as mapped_minimum)


@dataclass(frozen=True)
class Gathering:
    id: str
    tickets: tuple[str, ...]
    born: int
    host: str
    language: int
    game: Game | None = None
    clock: object | None = None


@dataclass(frozen=True)
class Query:
    ids: tuple[str, ...]
    started: float
    selector: object
    attempted: frozenset[str]
    finished: frozenset[str]
    generation: int


class _Clock:
    def __init__(self, owner, identity):
        self.owner, self.identity = owner, identity

    def snapshot(self):
        return self.owner._call('gather_clock', id=self.identity)

    @property
    def started(self): return self.snapshot()['started']
    @property
    def growth(self): return self.snapshot()['growth']
    @property
    def peak(self): return self.snapshot()['peak']

    def observe(self, now, players):
        return self.owner._fixture('clock', id=self.identity,
                                   command=dict(op='clock_observe', now=now, players=players))

    @growth.setter
    def growth(self, value):
        self.owner._fixture('clock', id=self.identity, command=dict(op='clock_growth', growth=value))


class _StateOwner:
    def __init__(self, owner, ticket):
        self.owner, self.ticket = owner, ticket

    def call(self, op, **values):
        return self.owner._call('gather_state_command', ticket=self.ticket, command=dict(op=op, **values))

    def snapshot(self): return self.call('snapshot')


class _Selector:
    def __init__(self, owner, ticket, generation):
        self.owner, self.ticket, self.generation = owner, ticket, generation

    @property
    def bank(self):
        return bytes.fromhex(self.owner._call('gather_query_bank', ticket=self.ticket, generation=self.generation))


class _Map(Mapping):
    def __init__(self, owner, name):
        self.owner, self.name = owner, name

    def __iter__(self):
        return iter(()) if self.owner.core is None else iter(self.owner._call('gather_keys', map=self.name))

    def __len__(self): return len(tuple(iter(self)))

    def __getitem__(self, key):
        try:
            data = self.owner._call('gather_get', map=self.name, key=key)
        except Problem as error:
            if error.status == 404:
                raise KeyError(key) from None
            raise
        if self.name == 'sessions':
            return self.owner._gathering(data)
        if self.name == 'states':
            return self.owner._state_view(key)
        return Query(tuple(data['ids']), data['started'], _Selector(self.owner, key, data['generation']),
                     frozenset(data['attempted']), frozenset(data['finished']), data['generation'])


class GatheringSessions:
    def __init__(self, api, *, wall_clock=time.time, seed=None):
        self.api, self.core, self.qos = api, api.core, api.search_qos
        self.wall_clock = wall_clock
        self.sessions, self.queries, self.states = (_Map(self, name) for name in ('sessions', 'queries', 'states'))
        if self.core is not None:
            self._call('gather_init', seed=secrets.randbits(32) if seed is None else seed)

    def _call(self, op, **values):
        require(self.core is not None, 'Gathering playlists are not configured', 503)
        return self.core._call(op, **values)

    def _gathering(self, data):
        if data is None:
            return None
        return Gathering(data['id'], tuple(data['tickets']), data['born'], data['host'], data['language'],
                         project('games', data['game']), None if data['clock'] is None else _Clock(self, data['id']))

    def _state_view(self, ticket):
        result = object.__new__(PartyState)
        result._owner = _StateOwner(self, ticket)
        return result

    def _pump(self, event):
        while event['kind'] == 'io':
            frame = event['frame']
            try:
                if event['action'] == 'facts':
                    result = dict(ready=[bool(self.qos.ready(p)) for p in event['players']], pairs=[])
                    for a, b in event['pairs']:
                        measured = self.qos.measurement(a, b)
                        result['pairs'].append(dict(milliseconds=None if measured is None else measured.milliseconds,
                                                    pending=bool(self.qos.pending(a, b))))
                else:
                    require(event['action'] == 'request', 'Unknown Rust transport request', 503)
                    result = [bool(self.qos.request(a, b)) for a, b in event['pairs']]
                event = self._call('gather_resume', frame=frame, result=result)
            except BaseException:
                # Retire only this unfinished IO continuation. Previously sent
                # real probes remain owned by the transport until completion.
                try:
                    self._call('gather_abort', frame=frame)
                except Problem:
                    pass
                raise
        require(event['kind'] == 'done', 'Invalid Rust gathering response', 503)
        return event['value']

    @property
    def seed(self): return self._call('gather_seed')

    @staticmethod
    def tick(now): return value('gather_tick', value=now)

    def cleanup(self):
        if self.core is not None:
            self.core.cleanup()
            self._call('gather_cleanup')

    def group(self, gathering):
        return tuple(project('tickets', t) for t in self._call('gather_group', id=gathering.id))

    def progress(self, ticket, viewer):
        if ticket is None:
            return None
        return self._call('gather_progress', ticket=ticket.id, viewer=viewer)

    def fits(self, policy, group, ticket, game=None):
        return self.core.fits(policy, group, ticket, game)

    def leader(self, ticket): return self._call('gather_leader', ticket=ticket.id)

    def state(self, ticket):
        self._call('gather_state', ticket=ticket.id)
        return self._state_view(ticket.id)

    def start_search(self, policy, ticket, state, tick):
        self._call('gather_start_search', ticket=ticket.id, tick=tick)

    def withdraw(self, gathering): self._call('gather_withdraw', id=gathering.id)

    def search(self, policy, ticket, progress=None):
        data = self._call('gather_search', playlist=policy.id, ticket=ticket.id,
                          progress=None if progress is None else progress.snapshot())
        search = data['search']
        for name in ('clan', 'own_address'):
            if search[name] is not None:
                search[name] = bytes.fromhex(search[name])
        return _construct(Search, search), data['threshold']

    def bank(self, policy, ticket, gatherings, excluded=()):
        return bytes.fromhex(self._pump(self._call('gather_bank', playlist=policy.id, ticket=ticket.id,
            ids=tuple(g.id for g in gatherings), excluded=tuple(excluded), wall=self.wall_clock())))

    def found(self, policy, ticket, state, now):
        return self._gathering(self._call('gather_found', playlist=policy.id, ticket=ticket.id, wall=self.wall_clock()))

    def proposal(self, policy, gathering):
        return project('proposals', self._call('gather_proposal', playlist=policy.id, id=gathering.id))

    def query(self, policy, ticket, state, now):
        result = self._pump(self._call('gather_query', playlist=policy.id, ticket=ticket.id, wall=self.wall_clock()))
        return result['again'], self._gathering(result['gathering'])

    def advance(self, playlist):
        return project('proposals', self._pump(self._call('gather_advance', playlist=playlist, wall=self.wall_clock())))

    def _fixture(self, action, **values):
        """Explicit synthetic inputs, absent from the production Rust build."""
        require(native().h2_matchmaking_test_inputs_enabled(), 'Diagnostic gathering inputs are not compiled', 503)
        return self._call('gather_fixture', action=action, **values)

    def _restore_fixture(self, gathering):
        data = dict(id=gathering.id, tickets=gathering.tickets, born=gathering.born, host=gathering.host,
                    language=gathering.language, game=gathering.game,
                    clock=None if gathering.clock is None else gathering.clock.snapshot())
        return self._gathering(self._fixture('insert', value=data))

    def _fixture_members(self, gathering, tickets):
        return self._gathering(self._fixture('members', id=gathering.id, tickets=tickets))
