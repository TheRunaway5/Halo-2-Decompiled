"""Temporary typed IO for original Rust XP arithmetic and skill storage.

Rust owns rank tables, arithmetic, skill domains, snapshots and storage plans.
The generic SQLite adapter never accepts client XP.
"""
from dataclasses import dataclass
from .native_database import run as database

from .native_matchmaking import value

_metadata=value('rank_metadata')
POLICY,TABLE_DIGEST=_metadata['policy'],_metadata['table_digest']
LATER_POLICY,LATER_DIGEST=_metadata['later_policy'],_metadata['later_digest']
MAX_XP=_metadata['max_xp']


def rank_policy(name):
    result=value('rank_policy',name=name)
    return result['tables'],result['digest']


POLICIES={name:rank_policy(name) for name in value('rank_policies')}
TABLES=POLICIES[POLICY][0]


def divide(number,denominator):
    return value('rank_divide',value=number,denominator=denominator)


def pair_delta(own,other,place,other_place,tables=TABLES):
    return value('rank_pair',own=own,other=other,place=place,other_place=other_place,tables=tables)


@dataclass(frozen=True)
class RatingPlayer:
    skill: int
    place: int
    team: int
    xp: int = 5000
    key: int = 3
    flags: int = 0
    active: bool = True

def player_delta(players,selected,*,key=3,team_places=None,tables=TABLES):
    return value('rank_player',players=tuple(players),selected=selected,key=key,team_places=team_places,tables=tables)


def raw_rank(xp,previous,tables=TABLES):
    return value('rank_raw',xp=xp,previous=previous,tables=tables)


def display_level(raw):
    return value('rank_display',rank=raw)


@dataclass(frozen=True)
class Skill:
    raw: int
    xp: int
    configuration: str

class PlaylistSkills:
    """Typed IO to Rust-owned skill schemas, snapshots and migrations."""
    def __init__(self, db):
        self.db = db
        database(db, 'player_schema')

    def snapshot(self, player, playlist, configuration):
        return Skill(**database(self.db, 'player_snapshot', owner=player,
                               playlist=playlist, configuration=configuration))
