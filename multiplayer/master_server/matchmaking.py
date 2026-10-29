"""Immutable service carriers for the Rust matchmaking authority.

The native library validates catalogs and players and owns every decision and
all queue/proposal state. Python transports trusted identities and snapshots.
"""
from dataclasses import dataclass
from fractions import Fraction

from .protocol import require
from .search_lifecycle import GatheringConfiguration
from .native_matchmaking import Coordinator, Defaults, value


@dataclass(frozen=True)
class PartyBalance:
    minimum_large: int
    maximum: int
    imbalance: int


@dataclass(frozen=True)
class ClanMatch:
    minimum: int
    maximum: int
    imbalance: int


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
    settings: str | None = None  # Complete admitted 304-byte option buffer, never inferred by name.


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
    hopper: str | None = None  # Complete original-layout header; authored metadata stays versioned.

    def __post_init__(self):
        require(type(self.games) is tuple and all(type(g) is Game for g in self.games), 'Typed game tuple required')
        require(type(self.stages) is tuple and all(type(s) is SearchStage and type(s.rank_fraction) is Fraction
                for s in self.stages), 'Typed search stage tuple required')
        require(self.gathering is None or type(self.gathering) is GatheringConfiguration, 'Typed gathering required')
        require(self.party_balance is None or type(self.party_balance) is PartyBalance, 'Typed party balance required')
        require(self.clan_match is None or type(self.clan_match) is ClanMatch, 'Typed clan rule required')
        value('validate', kind='playlist', value=self)


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
        require(type(self.maps) is frozenset, 'Typed installed-map set required')
        value('validate', kind='player', value=self)

    @property
    def search_rank(self):
        return value('search_rank', player=self)


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


PARTY_RULES_2007 = Defaults('party_rules_2007', PartyBalance)
CLAN_RULES_RETAIL = Defaults('clan_rules_retail', ClanMatch)


def rank_radius_2007(rank, *, high_rank_radius=None):
    return value('rank_radius', rank=rank, high=high_rank_radius)


def effective_ranks(playlist, players):
    return tuple(value('effective_ranks', policy=playlist, players=tuple(players)))


def party_rank(playlist, players):
    return value('party_rank', policy=playlist, players=tuple(players))
