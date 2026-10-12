"""Native gathering allocator feeding the recovered retail candidate selector.

This translates the recovered client lifecycle onto an explicit native query
service and relay, without claiming Xbox Live transport parity. A gathering has its own
identity, host, birth wall-clock and current admitted whole parties. Selection
uses its actual advertisement and a server-measured native relay-route probe.
No completed central subset, ticket age or normalized relay score is substituted
for those original scorer inputs. See ../docs/matchmaking.md.
"""
from dataclasses import dataclass, field
import hashlib
import struct
import secrets
import time
import uuid

from .matchmaking import Game, Proposal, party_rank
from .session_candidates import BANK_SIZE, STRIDE, Candidates, Search, threshold
from .search_lifecycle import GatheringClock, SearchProgress, SeenSessions, elapsed, jitter, role


RETAIL_RANK_LIMITS = tuple(min(127, rank+(5 if rank <= 5 else 6 if rank <= 15 else
    7 if rank <= 25 else 8 if rank <= 30 else 9 if rank <= 35 else 10)) for rank in range(128))


def mapped_minimum(minimum, maximum):
    """0x73530 floor using matched0x1931A0 and launch0x1936E0 table."""
    if type(minimum) is not int or type(maximum) is not int or not 0 <= minimum <= maximum <= 127:
        raise ValueError("Invalid trusted gathering ranks")
    lower = maximum
    while lower > 0 and RETAIL_RANK_LIMITS[lower-1] >= maximum:
        lower -= 1
    return max(minimum, lower)


def rank_window(minimum, maximum, radius=None):
    """Original0x90880 initialization and0x90B30 bound arithmetic.

    The caller owns query/radius scheduling. None requests the full window.
    """
    base = mapped_minimum(minimum, maximum)
    floor = mapped_minimum(0, maximum)
    ceiling = RETAIL_RANK_LIMITS[base]
    maximum_radius = max(base-floor, ceiling-maximum)
    if radius is None:
        radius = maximum_radius
    if type(radius) is not int or not 0 <= radius <= maximum_radius:
        raise ValueError("Invalid gathering search radius")
    return maximum_radius, max(base-radius, floor), min(maximum+radius, ceiling)


@dataclass
class Gathering:
    id: str
    tickets: tuple[str, ...]
    born: int
    host: str
    language: int
    game: Game | None = None
    clock: GatheringClock | None = None


@dataclass
class PartyState:
    mode: str = "choose"
    search_start: int = 0
    gather_start: int = 0
    search_offset: int = 0
    gather_offset: int = 0
    progress: SearchProgress | None = None
    seen: SeenSessions = field(default_factory=SeenSessions)


@dataclass
class Query:
    ids: tuple[str, ...]
    started: float
    selector: Candidates
    attempted: set[str] = field(default_factory=set)
    finished: set[str] = field(default_factory=set)


class GatheringSessions:
    def __init__(self, api, *, wall_clock=time.time, seed=None):
        self.api, self.core, self.qos = api, api.core, api.search_qos
        self.wall_clock = wall_clock
        self.seed = secrets.randbits(32) if seed is None else seed
        self.sessions, self.queries, self.states = {}, {}, {}

    def cleanup(self):
        if self.core is None:
            return
        self.core.cleanup()
        reserved = {key for p in self.core.proposals.values() for key in p.tickets}
        available = set(self.core.tickets)-reserved
        for key, gathering in tuple(self.sessions.items()):
            # Native immutable-party admission: any queue withdrawal destroys
            # this gathering. Surviving parties can enter a newly born one.
            if not set(gathering.tickets) <= available:
                self.withdraw(gathering)
        self.states = {k:v for k,v in self.states.items() if k in available}
        self.queries = {k: v for k, v in self.queries.items() if k in available}

    def group(self, gathering):
        return tuple(self.core.tickets[key] for key in gathering.tickets)

    def progress(self, ticket, viewer):
        """Public display only: read the admitted gathering's existing clock.

        Do not advance a search, sample teams/RNG, observe a new peak, expose
        candidate ranks or return private gathering/host/player identities.
        """
        if ticket is None:
            return None
        gathering = next((g for g in self.sessions.values() if ticket.id in g.tickets),None)
        if gathering is None:
            return None
        policy = self.core.playlists[ticket.playlist]
        players = tuple(p for t in self.group(gathering) for p in t.players)
        count = len(players)
        countdown = None
        if policy.minimum_players <= count <= policy.maximum_players and gathering.clock is not None:
            remaining = policy.gathering.delay(count,policy.minimum_players,policy.maximum_players)-elapsed(
                self.tick(self.core.now()),gathering.clock.growth)
            if remaining > 0:
                countdown = remaining
        return {"players":count,"minimum_players":policy.minimum_players,
                "maximum_players":policy.maximum_players,"wait_ms":countdown,
                "roster":[{"name":p.screen_name,"self":p.id == viewer} for p in players]}

    def fits(self, policy, group, ticket):
        if any(not self.core._compatible(policy, t, ticket) for t in group):
            return False
        sizes = [len(t.players) for t in (*group, ticket)]
        if sum(sizes) > policy.maximum_players:
            return False
        if policy.clan_match:
            return len(sizes) == 2
        if not policy.team_size:
            return True
        # Keep every admitted party intact. A partial gathering must still fit
        # two teams; requiring a complete central subset here would recreate
        # the old assembly-before-selection behavior.
        possible = {0}
        for size in sizes:
            possible |= {n+size for n in tuple(possible) if n+size <= policy.team_size}
        return any(sum(sizes)-n <= policy.team_size for n in possible)

    @staticmethod
    def leader(ticket):
        return ticket.leader or ticket.players[0].id

    @staticmethod
    def clan(value):
        return hashlib.sha256(value.encode()).digest()[:12] if value else None

    def state(self, ticket):
        if ticket.id not in self.states:
            self.seed, search_offset, gather_offset = jitter(self.seed)
            self.states[ticket.id] = PartyState(search_offset=search_offset, gather_offset=gather_offset)
        return self.states[ticket.id]

    @staticmethod
    def tick(now):
        return int(now*1000) & 0xffffffff

    def start_search(self, policy, ticket, state, tick):
        ranks = [ticket.clan_rank] if ticket.clan else [p.search_rank for p in ticket.players]
        state.mode, state.search_start = "search", tick
        state.progress = SearchProgress(rank_window(min(ranks),max(ranks))[0], policy.ranked)
        # This is a fresh successful search initialization (0x90A4F), not
        # merely a role transition. Radius/same-radius retries retain the bank.
        state.seen.clear()
        self.queries.pop(ticket.id,None)

    def withdraw(self, gathering):
        self.sessions.pop(gathering.id,None)
        # Native reservation work is serialized and atomic. There is no Xbox
        # member connection to drain here; do not fake a platform acknowledgment.
        for index, key in enumerate(gathering.tickets):
            self.queries.pop(key,None)
            ticket, state = self.core.tickets.get(key), self.states.get(key)
            if ticket is not None and state is not None:
                if index == 0:
                    state.mode = "choose"
                else:
                    self.start_search(self.core.playlists[ticket.playlist],ticket,state,self.tick(self.core.now()))

    def search(self, policy, ticket, progress=None):
        average = ticket.clan_rank if ticket.clan else party_rank(policy, ticket.players)
        ranks = [ticket.clan_rank] if ticket.clan else [p.search_rank for p in ticket.players]
        maximum = rank_window(min(ranks),max(ranks))[0]
        radius = progress.radius if progress is not None else 0
        bounds = (None,None) if radius == -1 else rank_window(min(ranks),max(ranks),radius)[1:]
        # Fresh/unmeasured quality is legitimately (0,-1). Operational fallback
        # bandwidth is never advertised as a measured capacity.
        return Search(len(ticket.players), average, ticket.language, 0, -1,
                      *bounds, clan=self.clan(ticket.clan)), threshold(radius,maximum)

    def bank(self, policy, ticket, gatherings, excluded=()):
        bank = bytearray(BANK_SIZE)
        for index, gathering in enumerate(gatherings):
            group = self.group(gathering)
            players = tuple(p for t in group for p in t.players)
            ranks = ([t.clan_rank for t in group for _ in t.players] if policy.clan_match else
                     [p.search_rank for p in players])
            average = sum(ranks)//len(ranks)
            minimum = mapped_minimum(min(ranks), max(ranks))
            space = policy.maximum_players-len(players)
            mask = (1 << min(space, policy.maximum_party))-1
            age = int(self.wall_clock())-gathering.born
            measured = self.qos.measurement(self.leader(ticket), gathering.host)
            row = memoryview(bank)[index*STRIDE:(index+1)*STRIDE]
            row[0] = 1
            row[1] = int(gathering.id not in excluded and
                         gathering.id not in (self.states[ticket.id].seen if ticket.id in self.states else ()))
            row[0x24:0x48] = gathering.id.encode("ascii")
            def put(offset, value):
                struct.pack_into("<i", row, offset, value)
            # Brief service attributes exist while the full route/description
            # is pending. Only the successful probe installs a ready detail.
            for offset, value in ((0x5C, average), (0x60, space), (0x64, minimum),
                                  (0x68, max(ranks)), (0x6C, age), (0x70, mask)):
                put(offset, value)
            if measured is not None:
                row[0x74] = row[0xA4] = 1
                # Original type/version fields are an internal compatibility
                # translation after exact native playlist/revision/protocol
                # admission. These bytes are never a public Xbox protocol.
                for offset, value in ((0x90, measured.milliseconds), (0xAC, 4), (0xB0, 0x2651),
                                      (0xB4, 0x2651), (0xB8, gathering.language), (0xF8, 0), (0xFC, -1),
                                      (0x10C, space), (0x110, mask), (0x11C, age), (0x120, average),
                                      (0x124, max(ranks)), (0x12C, minimum)):
                    put(offset, value)
                clans = tuple(dict.fromkeys(t.clan for t in group if t.clan))
                put(0x130, len(clans))
                for i, clan in enumerate(clans):
                    row[0x134+i*12:0x140+i*12] = self.clan(clan)
        return bytes(bank)

    def found(self, policy, ticket, state, now):
        tick = self.tick(now)
        state.mode, state.gather_start = "gather", tick
        game = self.core.rng.choices(policy.games, weights=[g.weight for g in policy.games],k=1)[0]
        gathering = Gathering(str(uuid.uuid4()),(ticket.id,),int(self.wall_clock()),
                              self.leader(ticket),ticket.language,game,GatheringClock(tick,tick))
        gathering.clock.observe(tick,len(ticket.players))
        self.sessions[gathering.id] = gathering
        self.queries.pop(ticket.id,None)
        return gathering

    def proposal(self, policy, gathering):
        group = self.group(gathering)
        count = sum(len(t.players) for t in group)
        tick = self.tick(self.core.now())
        if gathering.clock is None:
            gathering.clock = GatheringClock(tick,tick)
        waited = gathering.clock.observe(tick,count)
        if (not policy.minimum_players <= count <= policy.maximum_players or
                policy.gathering.delay(count,policy.minimum_players,policy.maximum_players) > waited):
            return None
        teams = self.core._teams(policy,group)
        if teams is None:
            return None
        game = gathering.game or self.core.rng.choices(policy.games,weights=[g.weight for g in policy.games],k=1)[0]
        proposal = Proposal(str(uuid.uuid4()),policy.id,policy.revision,gathering.tickets,
                            teams,None,game,self.core.now()+self.core.proposal_lease,gathering.host)
        self.core.proposals[proposal.id] = proposal
        self.core.ready[proposal.id] = set()
        del self.sessions[gathering.id]
        return proposal

    def query(self, policy, ticket, state, now):
        search, minimum = self.search(policy,ticket,state.progress)
        query = self.queries.get(ticket.id)
        if query is None:
            # The missing Live procedure is replaced explicitly: current
            # structurally compatible sessions ordered by birth/private ID,
            # at most50. Synchronous completion permits bounded empty-query
            # progress in this request, rather than a fabricated timer/task.
            candidates = sorted((g for g in self.sessions.values()
                if self.fits(policy,self.group(g),ticket)),key=lambda g:(g.born,g.id))[:50]
            query = Query(tuple(g.id for g in candidates),now,Candidates(search))
            self.queries[ticket.id] = query
        candidates = [self.sessions[k] for k in query.ids if k in self.sessions and
                      self.fits(policy,self.group(self.sessions[k]),ticket)]
        query.selector.update(self.bank(policy,ticket,candidates,query.finished))
        pending = False
        for index in range(50):
            row = query.selector.bank[index*STRIDE:(index+1)*STRIDE]
            if not row[0] or not row[1] or row[0x74]:
                continue
            key = row[0x24:0x48].decode("ascii")
            gathering = self.sessions[key]
            if key in query.finished:
                continue
            if not all(self.qos.ready(p.id) for t in self.group(gathering) for p in t.players):
                query.finished.add(key)
                state.seen.reject(key)
                continue
            a,b = self.leader(ticket),gathering.host
            if key in query.attempted and not self.qos.pending(a,b):
                query.finished.add(key)
                state.seen.reject(key)
                continue
            if self.qos.request(a,b):
                query.attempted.add(key)
            if key not in query.attempted and now-query.started >= 12:
                # Native bounded allocation wait, distinct from an original
                # query cadence or a claimed XNet QoS timeout.
                query.finished.add(key)
                state.seen.reject(key)
            else:
                pending = True
        if query.finished:
            # Completed failed targets are ineligible and remembered across
            # subsequent radius queries, as in0x905A0/0x90650.
            query.selector.update(self.bank(policy,ticket,candidates,query.finished))
        selected = query.selector.select(minimum,int((now-query.started)*1000))
        if selected is not None:
            gathering = self.sessions[selected[0x24:0x48].decode("ascii")]
            gathering.tickets += (ticket.id,)
            state.mode = "joined"
            self.queries.pop(ticket.id,None)
            return False,gathering
        if pending:
            return False,None
        again = state.progress.complete(len(query.ids))
        self.queries.pop(ticket.id,None)
        if not again:
            state.mode = "choose"
        return True,None

    def advance(self, playlist):
        self.cleanup()
        policy, now = self.core.playlists[playlist],self.core.now()
        if policy.gathering is None:
            raise ValueError("Gathering configuration required")
        tick = self.tick(now)
        # Existing gatherers launch or exhaust before new advertisements are
        # considered. A new high count resets only the growth/countdown clock.
        for gathering in tuple(self.sessions.values()):
            if self.group(gathering)[0].playlist != playlist:
                continue
            proposal = self.proposal(policy,gathering)
            if proposal:
                return proposal
            founder = self.core.tickets[gathering.tickets[0]]
            state = self.state(founder)
            if gathering.clock.expired(tick,policy.gathering,state.gather_offset):
                self.withdraw(gathering)
        reserved = {key for p in self.core.proposals.values() for key in p.tickets}
        queue = sorted((t for t in self.core.tickets.values() if t.playlist == playlist and t.id not in reserved),
                       key=lambda t:(t.joined_at,t.id))
        budget = self.core.search_budget
        for ticket in queue:
            if any(ticket.id in g.tickets for g in self.sessions.values()):
                continue
            if not all(self.qos.ready(p.id) for p in ticket.players):
                continue
            state = self.state(ticket)
            while budget:
                query = self.queries.get(ticket.id)
                cost = max(1,len(query.ids) if query is not None else min(50,len(self.sessions)))
                if budget < cost:
                    break
                budget -= cost
                if state.mode == "choose":
                    if role(tick,state.search_start,state.gather_start,policy.maximum_players) == "search":
                        self.start_search(policy,ticket,state,tick)
                    else:
                        gathering = self.found(policy,ticket,state,now)
                        proposal = self.proposal(policy,gathering)
                        if proposal:
                            return proposal
                        break
                if state.mode != "search":
                    break
                if elapsed(tick,state.search_start) > 240000+state.search_offset:
                    self.queries.pop(ticket.id,None)
                    state.mode = "choose"
                    continue
                again,gathering = self.query(policy,ticket,state,now)
                if gathering:
                    proposal = self.proposal(policy,gathering)
                    if proposal:
                        return proposal
                if not again:
                    break
        return None
