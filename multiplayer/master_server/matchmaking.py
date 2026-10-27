"""Trusted-input matchmaking coordinator, independent of directory discovery.

See ../docs/halo2-matchmaking-evidence.md and matchmaking.md. This implements
party queues and match proposals, not an Xbox Live protocol or game admission.
Ratings, membership and connection measurements must come from trusted services;
this module is deliberately not exposed as a client-writable HTTP endpoint.
"""
from dataclasses import dataclass
from fractions import Fraction
from itertools import combinations
import math
import random
import time
import uuid

from .protocol import integer, require, text
from .search_lifecycle import GatheringConfiguration


def rank_radius_2007(rank, *, high_rank_radius=None):
    """Bungie Stats Overview, Jan 2007. Its duplicated rank35 row is ambiguous."""
    integer(rank, 1, 50)
    if rank <= 6:
        return 11 - rank
    if rank <= 16:
        return 6
    if rank <= 26:
        return 7
    if rank <= 29:
        return 8
    if rank <= 35:
        return rank - 21
    require(high_rank_radius is not None, "Original high-rank table is ambiguous; explicit tuning required")
    # The duplicated final row gives an upward cap of50, not a lower bound.
    # Never derive a zero-width window for rank50 from that upward cap.
    return integer(high_rank_radius, 0, 49)


@dataclass(frozen=True)
class PartyBalance:
    minimum_large: int
    maximum: int
    imbalance: int


# These are the dated large-party rules, not team-size imbalance allowances.
PARTY_RULES_2007 = {
    "team_skirmish": PartyBalance(3, 4, 0),
    "big_team_battle": PartyBalance(3, 5, 2),
    "minor_clan_match": PartyBalance(3, 4, 0),
    "major_clan_match": PartyBalance(6, 8, 2),
    "double_team": PartyBalance(2, 2, 0),
    "team_preview": PartyBalance(3, 4, 0),
}


@dataclass(frozen=True)
class ClanMatch:
    minimum: int
    maximum: int
    imbalance: int


# Retail0x193C70/193C00 type5 defaults, enforced by66050/6D4C0/7E210.
# Later downloaded hoppers are independent configurations.
CLAN_RULES_RETAIL = {
    "minor-retail-launch-2004": ClanMatch(3, 4, 0),
    "major-retail-launch-2004": ClanMatch(6, 8, 2),
}


@dataclass(frozen=True)
class SearchStage:
    after_seconds: int
    rank_fraction: Fraction
    minimum_connection: int
    unbounded_skill: bool = False  # Retail casual's final retry; cadence stays explicit policy.


@dataclass(frozen=True)
class Game:
    id: str
    map: str
    variant: str
    weight: int


@dataclass(frozen=True)
class Playlist:
    id: str
    revision: str
    ranked: bool
    team_size: int  # Zero means free for all; clan hoppers may launch below this maximum.
    minimum_players: int
    maximum_players: int
    maximum_party: int
    games: tuple[Game, ...]
    stages: tuple[SearchStage, ...]
    provenance: str  # Required: exact historical source, or explicitly lab/custom.
    party_balance: PartyBalance | None = None
    use_2007_rank_limits: bool = False
    high_rank_radius: int | None = None  # Unverified36+ radius; no implicit default.
    mixed_party_floor: bool = False  # Inference from Bungie's12+1 ->12+6 example.
    custom_rank_radius: int | None = None
    clan_match: ClanMatch | None = None
    candidate_selection: str | None = None
    gathering: GatheringConfiguration | None = None
    team_count: int = 2  # Original generic balancer supports up to four teams.

    def __post_init__(self):
        require(self.candidate_selection in (None, "retail-candidates-native-relay-v1"), "Unknown candidate selection policy")
        require((self.candidate_selection is None and self.gathering is None) or
                (self.candidate_selection is not None and type(self.gathering) is GatheringConfiguration),
                "Gathering selection requires explicit countdown configuration")
        for value in (self.id, self.revision, self.provenance):
            text(value, 512)
        require(type(self.ranked) is bool, "Invalid ranked flag")
        integer(self.minimum_players, 2, 16)
        integer(self.maximum_players, self.minimum_players, 16)
        integer(self.team_size, 0, 8)
        integer(self.team_count, 2, 4)
        integer(self.maximum_party, 1, self.maximum_players if self.team_count>2 else self.team_size or self.maximum_players)
        require(not self.team_size or self.maximum_players == self.team_count * self.team_size,
                "Team playlist capacity differs from its configured team count")
        require(self.team_count==2 or (self.team_size and not self.ranked and not self.clan_match and self.party_balance is None),
                "Original multi-team assignment requires an explicit casual team policy")
        require(not self.team_size or self.clan_match is not None or self.minimum_players == self.maximum_players,
                "Reduced team counts require a separately specified policy")
        if self.clan_match is not None:
            require(self.clan_match in CLAN_RULES_RETAIL.values(), "Unknown clan lineup policy")
            rule = self.clan_match
            require(self.ranked and self.team_size == self.maximum_party == rule.maximum and
                    self.minimum_players == 2*rule.minimum and self.maximum_players == 2*rule.maximum and
                    self.party_balance is None and not self.mixed_party_floor and not self.use_2007_rank_limits,
                    "Clan playlist dimensions must match its recovered lineup policy")
        require(type(self.games) is tuple and 1 <= len(self.games) <= 256, "Invalid game pool")
        require(len({g.id for g in self.games}) == len(self.games), "Duplicate game entry")
        for game in self.games:
            for value in (game.id, game.map, game.variant):
                text(value)
            integer(game.weight, 1, 10000)
        require(type(self.stages) is tuple and 1 <= len(self.stages) <= 16, "Invalid search stages")
        previous_time, previous_fraction, previous_connection = -1, Fraction(0), 101
        for index, stage in enumerate(self.stages):
            integer(stage.after_seconds, 0, 3600)
            integer(stage.minimum_connection, 0, 100)
            require(type(stage.rank_fraction) is Fraction and 0 < stage.rank_fraction <= 1,
                    "Invalid rank expansion fraction")
            require(type(stage.unbounded_skill) is bool, "Invalid unrestricted skill flag")
            require(not stage.unbounded_skill or
                    (not self.ranked and index == len(self.stages)-1 and stage.after_seconds > 0),
                    "Only a final casual retry may remove skill bounds")
            require(stage.after_seconds > previous_time and stage.rank_fraction >= previous_fraction and
                    stage.minimum_connection <= previous_connection, "Search stages must progressively relax")
            previous_time, previous_fraction, previous_connection = (
                stage.after_seconds, stage.rank_fraction, stage.minimum_connection)
        require(self.stages[0].after_seconds == 0, "Search must start at zero")
        for flag in (self.use_2007_rank_limits, self.mixed_party_floor):
            require(type(flag) is bool, "Invalid skill policy flag")
        require(self.use_2007_rank_limits != (self.custom_rank_radius is not None),
                "Choose historical ranked limits or an explicitly custom skill radius")
        if self.custom_rank_radius is not None:
            integer(self.custom_rank_radius, 0, 49)
        if self.high_rank_radius is not None:
            integer(self.high_rank_radius, 0, 49)
            require(self.use_2007_rank_limits, "High-rank tuning requires the historical table")
        require(not self.mixed_party_floor or self.use_2007_rank_limits,
                "Mixed-party inference requires the corresponding historical table")
        if self.party_balance:
            integer(self.party_balance.minimum_large, 2, 8)
            integer(self.party_balance.maximum, self.party_balance.minimum_large, 8)
            integer(self.party_balance.imbalance, 0, 7)
            require(self.team_size and self.maximum_party == self.party_balance.maximum,
                    "Large-party policy must agree with playlist limits")


@dataclass(frozen=True)
class Player:
    id: str  # Opaque trusted identity, never a screen name or Discord subject.
    screen_name: str
    rank: int  # Visible artwork level; private raw skill drives configured XP policies.
    maps: frozenset[str]
    raw_rank: int | None = None  # Private retail XP rank; never replace it with artwork level.
    guest: bool = False
    connection_owner: str | None = None

    def __post_init__(self):
        text(self.id)
        text(self.screen_name)
        require(type(self.guest) is bool, "Invalid trusted guest flag")
        if self.connection_owner is not None:
            text(self.connection_owner)
        integer(self.rank, 1, 50)
        if self.raw_rank is not None:
            integer(self.raw_rank, 0, 127)
            require(self.rank == min(self.raw_rank, 49) + 1, "Display level differs from private skill")
        require(type(self.maps) is frozenset and 1 <= len(self.maps) <= 256, "Invalid installed-map set")
        for value in self.maps:
            text(value)

    @property
    def search_rank(self):
        # Explicit legacy/lab policies without recovered XP retain their
        # existing level units. Configured retail policies always supply raw.
        return self.rank if self.raw_rank is None else self.raw_rank


@dataclass(frozen=True)
class Ticket:
    id: str
    party_id: str
    playlist: str
    revision: str
    compatibility: str
    players: tuple[Player, ...]
    joined_at: float
    expires_at: float
    clan: str | None = None
    clan_rank: int | None = None
    leader: str | None = None
    language: int = 0


@dataclass(frozen=True)
class Proposal:
    id: str
    playlist: str
    revision: str
    tickets: tuple[str, ...]
    teams: tuple[tuple[str, ...], ...]
    host: str | None
    game: Game
    expires_at: float
    gathering_host: str | None = None  # Private hosted B owner, fixed before matching completes.


def effective_ranks(playlist, players):
    levels = [p.search_rank for p in players]
    if playlist.mixed_party_floor:
        highest = max(levels)
        displayed = min(highest, 49) + 1 if players[0].raw_rank is not None else highest
        floor = highest - rank_radius_2007(displayed, high_rank_radius=playlist.high_rank_radius)
        levels = [max(level, floor) for level in levels]
    return tuple(levels)


def party_rank(playlist, players):
    # Original0x72540 averages available unadjusted ranks using integer IDIV.
    # Team-balancing adjustments belong to their separate owner, not search.
    rated = tuple(p for p in players if not p.guest)
    require(bool(rated), "A couch party needs a signed-in player")
    return sum(p.search_rank for p in rated) // len(rated)


class Coordinator:
    """One authoritative queue owner. Never federate these ephemeral tickets.

    The caller verifies identities/parties, owns rank snapshots and supplies QoS.
    Match proposals reserve tickets until transport readiness or rejection. A
    completed readiness handshake only hands off a proposal; it does not start
    gameplay, update ranks or imply an available server slot.
    """
    def __init__(self, playlists, *, clock=time.monotonic, rng=None, lease_seconds=90,
                 proposal_seconds=30, qos_seconds=30, capacity=1000, search_budget=10000):
        self.playlists = {p.id: p for p in playlists}
        require(len(self.playlists) == len(playlists) and 1 <= len(playlists) <= 64, "Invalid playlist catalog")
        for value, low, high in ((lease_seconds, 1, 300), (proposal_seconds, 1, 120),
                                 (qos_seconds, 1, 120), (capacity, 1, 1000), (search_budget, 1, 100000)):
            integer(value, low, high)
        self.clock, self.rng = clock, rng or random.SystemRandom()
        self.lease, self.proposal_lease, self.qos_lease = lease_seconds, proposal_seconds, qos_seconds
        self.capacity, self.search_budget = capacity, search_budget
        self.tickets, self.proposals, self.ready, self.connections = {}, {}, {}, {}
        self.next_anchor = {}
        self._clock_floor = 0.

    def now(self):
        value = self.clock()
        require(type(value) in (int, float) and math.isfinite(value), "Invalid coordinator clock")
        self._clock_floor = max(self._clock_floor, value)
        return self._clock_floor

    def cleanup(self):
        now = self.now()
        expired = {key for key, ticket in self.tickets.items() if ticket.expires_at <= now}
        for key, proposal in list(self.proposals.items()):
            if proposal.expires_at <= now or expired.intersection(proposal.tickets):
                del self.proposals[key]
                del self.ready[key]
        for key in expired:
            del self.tickets[key]
        active = {p.id for t in self.tickets.values() for p in t.players}
        self.connections = {key: value for key, value in self.connections.items()
                            if value[1] > now and set(key) <= active}

    def enqueue(self, party_id, playlist_id, compatibility, players, *, clan=None, clan_rank=None, leader=None, language=0):
        self.cleanup()
        text(party_id); text(compatibility)
        require(playlist_id in self.playlists, "Unknown playlist", 404)
        policy = self.playlists[playlist_id]
        require(type(players) is tuple and 1 <= len(players) <= policy.maximum_party, "Party not eligible")
        require(all(type(p) is Player for p in players), "Trusted player records required")
        require(not (policy.ranked or policy.clan_match) or not any(p.guest for p in players),
                "Guests may use casual playlists only", 409)
        require(len({p.raw_rank is not None for p in players}) == 1, "Mixed search skill units")
        require(len({p.id for p in players}) == len(players), "Duplicate party member")
        require(leader is None or leader in {p.id for p in players}, "Party leader is not a member")
        integer(language, 0, 8)
        if policy.clan_match:
            text(clan)
            integer(clan_rank, 0, 127)
            require(policy.clan_match.minimum <= len(players) <= policy.clan_match.maximum,
                    "Party size does not meet the clan playlist limits")
        else:
            require(clan is None and clan_rank is None, "Clan ratings require a clan playlist")
        # Validate all original table rows before admission, including rank36+.
        if policy.use_2007_rank_limits:
            ranks = (clan_rank,) if policy.clan_match else (p.rank for p in players)
            for rank in ranks:
                rank_radius_2007(rank, high_rank_radius=policy.high_rank_radius)
        require(all(all(g.map in p.maps for g in policy.games) for p in players),
                "Party lacks a map required by this playlist")
        for ticket in self.tickets.values():
            if ticket.party_id == party_id:
                require((ticket.playlist, ticket.compatibility, ticket.players) ==
                        (playlist_id, compatibility, players), "Party already queued differently", 409)
                require((ticket.clan, ticket.clan_rank) == (clan, clan_rank), "Clan search changed", 409)
                require((ticket.leader, ticket.language) == (leader, language), "Search owner changed", 409)
                return ticket
            # Native serialization protects frozen clan XP from concurrent
            # result commits; it is not a recovered global Xbox restriction.
            require(not clan or ticket.clan != clan or ticket.playlist != playlist_id,
                    "This clan is already searching this playlist", 409)
            require(not {p.id for p in ticket.players}.intersection(p.id for p in players),
                    "Player already queued", 409)
        require(len(self.tickets) < self.capacity, "Queue capacity reached", 503)
        now = self.now()
        ticket = Ticket(str(uuid.uuid4()), party_id, policy.id, policy.revision,
                        compatibility, players, now, now + self.lease, clan, clan_rank, leader, language)
        self.tickets[ticket.id] = ticket
        return ticket

    def heartbeat(self, ticket_id):
        self.cleanup()
        require(ticket_id in self.tickets, "Ticket not found", 404)
        from dataclasses import replace
        self.tickets[ticket_id] = replace(self.tickets[ticket_id], expires_at=self.now() + self.lease)

    def cancel(self, ticket_id):
        self.cleanup()
        require(ticket_id in self.tickets, "Ticket not found", 404)
        for key, proposal in list(self.proposals.items()):
            if ticket_id in proposal.tickets:
                del self.proposals[key]
                del self.ready[key]
        del self.tickets[ticket_id]
        self.cleanup()

    def connection(self, host, player, score):
        self.cleanup()
        active = {p.id for t in self.tickets.values() for p in t.players}
        require(host in active and player in active, "QoS identity is not queued", 404)
        integer(score, 0, 100)
        require((host, player) in self.connections or len(self.connections) < self.capacity * 16 * 50,
                "Connection measurement capacity reached", 503)
        # This score is an adapter contract, not an invented retail RTT formula.
        self.connections[host, player] = score, self.now() + self.qos_lease

    def _stage(self, policy, ticket):
        elapsed = self.now() - ticket.joined_at
        return next(stage for stage in reversed(policy.stages) if elapsed >= stage.after_seconds)

    def _radius(self, policy, rank, *, raw=False):
        if policy.use_2007_rank_limits:
            # This later published table uses visible levels. The average
            # retains raw units; conversion is limited to this table lookup.
            displayed = min(rank, 49) + 1 if raw else rank
            return rank_radius_2007(displayed, high_rank_radius=policy.high_rank_radius)
        return policy.custom_rank_radius

    def _compatible(self, policy, a, b):
        if (a.playlist, a.revision, a.compatibility) != (b.playlist, b.revision, b.compatibility):
            return False
        if (a.players[0].raw_rank is None) != (b.players[0].raw_rank is None):
            return False
        if policy.clan_match:
            if (not a.clan or not b.clan or a.clan == b.clan or
                    abs(len(a.players)-len(b.players)) > policy.clan_match.imbalance):
                return False
        if policy.candidate_selection:
            # The gathering owner applies original mapped extrema admission
            # to the advertised whole session. An average-distance prefilter
            # would discard candidates before that owner can evaluate them.
            return True
        if policy.clan_match:
            x, y = Fraction(a.clan_rank), Fraction(b.clan_rank)
        else:
            x, y = party_rank(policy, a.players), party_rank(policy, b.players)
        # Reciprocal enforcement is native policy: an old search cannot widen a
        # just-arrived party's bounds. Retail0x90F70/90B30 removes rank filters
        # on the final casual retry, retaining its composite candidate scoring.
        # Native measured-QoS admission remains enforced separately in _host.
        stages = self._stage(policy, a), self._stage(policy, b)
        raw = a.players[0].raw_rank is not None
        return ((stages[0].unbounded_skill or abs(x-y) <= self._radius(policy, x, raw=raw)*stages[0].rank_fraction) and
                (stages[1].unbounded_skill or abs(x-y) <= self._radius(policy, y, raw=raw)*stages[1].rank_fraction))

    def _teams(self, policy, group):
        if policy.clan_match:
            if len(group) != 2 or not self._compatible(policy, *group):
                return None
            return tuple(tuple(p.id for p in ticket.players) for ticket in group)
        if not policy.team_size:
            return tuple((p.id,) for ticket in group for p in ticket.players)
        if policy.team_count>2:
            from .team_balance import balance, can_add
            players=[player for ticket in group for player in ticket.players]
            require(all(p.raw_rank is not None for p in players),
                    "Original multi-team balancing requires authenticated raw ratings")
            parties=[index for index,ticket in enumerate(group) for _ in ticket.players]
            # Original7E210 first[] at+D4 supplies unadjusted raw ratings to
            # final gathering71730 ->91FC0, not adjusted party search ranks.
            weights=[p.raw_rank for p in players]
            for preserve in (True,False):
                # Casual gathering first tries party preservation, then retries
                # original balancing with individual seats if that cannot fit.
                if not can_add(policy.team_count,policy.team_size,parties,use_parties=preserve):continue
                assignment=balance(policy.team_count,policy.team_size,parties,weights,use_parties=preserve)
                if assignment is not None:
                    return tuple(tuple(p.id for p,t in zip(players,assignment) if t==team)
                                 for team in range(policy.team_count))
            return None
        partitions = []
        # Fix the first party on team0 to avoid mirrored duplicate partitions.
        for count in range(len(group)):
            for rest in combinations(group[1:], count):
                left = (group[0],) + rest
                if sum(len(t.players) for t in left) != policy.team_size:
                    continue
                left_ids = {t.id for t in left}
                right = tuple(t for t in group if t.id not in left_ids)
                rule = policy.party_balance
                if rule:
                    a = sorted(len(t.players) for t in left if len(t.players) >= rule.minimum_large)
                    b = sorted(len(t.players) for t in right if len(t.players) >= rule.minimum_large)
                    # One-to-one matching of multiple significant parties is
                    # a conservative reconstruction; published examples cover
                    # one large party per team, not every BTB composition.
                    if len(a) != len(b) or any(abs(x - y) > rule.imbalance for x, y in zip(a, b)):
                        continue
                # Fairness implementation, not recovered retail optimizer.
                difference = abs(sum(sum(effective_ranks(policy, t.players)) for t in left) -
                                 sum(sum(effective_ranks(policy, t.players)) for t in right))
                teams = tuple(tuple(p.id for t in team for p in t.players) for team in (left, right))
                partitions.append((difference, teams))
        if not partitions:
            return None
        best = min(difference for difference, _ in partitions)
        return self.rng.choice([teams for difference, teams in partitions if difference == best])

    def _host(self, policy, group, preferred=None, *, required=None, host_quality=None):
        now = self.now()
        if any(ticket.expires_at <= now for ticket in group):
            return None
        players = [p.id for t in group for p in t.players]
        minimum = {p.id: self._stage(policy, t).minimum_connection for t in group for p in t.players}
        hosts = []
        for host in players:
            if required is not None and host != required:
                continue
            if host_quality is not None and host not in host_quality:
                continue
            scores = [self.connections.get((host, p)) for p in players if p != host]
            if all(value is not None and value[1] > now and value[0] >= minimum[p]
                   for p, value in zip((p for p in players if p != host), scores)):
                hosts.append((min(value[0] for value in scores), host))
        if not hosts:
            return None
        if host_quality is not None:
            # Native measured delivery policy, not a recovered Xbox formula.
            # The transport supplies only physical connection owners with
            # reciprocal measured paths; playlist thresholds still apply.
            return min((host for _, host in hosts), key=host_quality.__getitem__)
        # Maximise the worst measured link; tie-break randomly. This host
        # ranking is an explicitly documented approximation, not retail code.
        best = max(score for score, _ in hosts)
        eligible = [host for score, host in hosts if score == best]
        return preferred if preferred in eligible else self.rng.choice(eligible)

    def advance(self, playlist_id, *, allow_pending_connectivity=False):
        self.cleanup()
        require(playlist_id in self.playlists, "Unknown playlist", 404)
        policy = self.playlists[playlist_id]
        require(policy.candidate_selection is None, "Use the gathering-session owner for this playlist")
        reserved = {key for p in self.proposals.values() for key in p.tickets}
        queue = sorted((t for t in self.tickets.values() if t.playlist == playlist_id and t.id not in reserved),
                       key=lambda t: (t.joined_at, t.id))
        if not queue:
            return None
        start_anchor = next((i for i, t in enumerate(queue) if t.id == self.next_anchor.get(playlist_id)), 0)
        queue = queue[start_anchor:] + queue[:start_anchor]
        budget = self.search_budget
        for anchor_index, anchor in enumerate(queue):
            self.next_anchor[playlist_id] = queue[(anchor_index + 1) % len(queue)].id
            # Original client searches requested at most50results. This is a
            # bounded candidate window, not a recovered server-side pool limit.
            candidates = [t for t in queue if t.id != anchor.id and self._compatible(policy, anchor, t)]
            pool = self.rng.sample(candidates, min(49, len(candidates)))
            anchor_budget = min(512, budget)

            def subsets(group, start, count):
                nonlocal budget, anchor_budget
                if not budget or not anchor_budget:
                    return
                budget -= 1
                anchor_budget -= 1
                yield group, count
                if count >= policy.maximum_players:
                    return
                for i in range(start, len(pool)):
                    if not budget or not anchor_budget:
                        return
                    ticket = pool[i]
                    if count + len(ticket.players) <= policy.maximum_players and all(
                            self._compatible(policy, other, ticket) for other in group):
                        yield from subsets(group + (ticket,), i + 1, count + len(ticket.players))

            for group, count in subsets((anchor,), 0, len(anchor.players)):
                if policy.minimum_players <= count <= policy.maximum_players:
                    teams = self._teams(policy, group)
                    host = self._host(policy, group) if teams else None
                    if host or (teams and allow_pending_connectivity):
                        game = self.rng.choices(policy.games, weights=[g.weight for g in policy.games], k=1)[0]
                        proposal = Proposal(str(uuid.uuid4()), policy.id, policy.revision,
                                            tuple(t.id for t in group), teams, host, game,
                                            self.now() + self.proposal_lease)
                        self.proposals[proposal.id] = proposal
                        self.ready[proposal.id] = set()
                        return proposal
            if not budget:
                break
        return None

    def refresh_connections(self, proposal_id, *, optimize_host=False, host_quality=None):
        self.cleanup()
        require(proposal_id in self.proposals, "Proposal not found", 404)
        proposal = self.proposals[proposal_id]
        group = tuple(self.tickets[key] for key in proposal.tickets)
        host = self._host(self.playlists[proposal.playlist], group, proposal.host,
                          required=None if optimize_host else proposal.gathering_host,
                          host_quality=host_quality)
        if host != proposal.host:
            from dataclasses import replace
            self.proposals[proposal_id] = replace(proposal, host=host)
            self.ready[proposal_id].clear()
        return self.proposals[proposal_id]

    def acknowledge(self, proposal_id, player_id, *, optimize_host=False, host_quality=None):
        self.cleanup()
        require(proposal_id in self.proposals, "Proposal not found", 404)
        proposal = self.refresh_connections(proposal_id, optimize_host=optimize_host, host_quality=host_quality)
        require(proposal.host is not None, "Connectivity has not been verified", 409)
        members = {p.id for key in proposal.tickets for p in self.tickets[key].players}
        require(player_id in members, "Player is not in this proposal", 403)
        self.ready[proposal_id].add(player_id)
        if self.ready[proposal_id] != members:
            return None
        # The trusted transport allocator consumes this handoff. It must own
        # admission/result reporting; the directory's player count is not used.
        for key in proposal.tickets:
            del self.tickets[key]
        del self.proposals[proposal_id]
        del self.ready[proposal_id]
        self.cleanup()
        return proposal

    def status(self, ticket_id):
        self.cleanup()
        require(ticket_id in self.tickets, "Ticket not found", 404)
        for proposal in self.proposals.values():
            if ticket_id in proposal.tickets:
                return {"state": "awaiting_transport" if proposal.host else "checking_connections", "proposal_id": proposal.id,
                        "expires_in": max(0, math.ceil(proposal.expires_at - self.now()))}
        return {"state": "searching"}
