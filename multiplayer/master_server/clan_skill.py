"""Clan-owned hopper XP: retail 0x74720/741F0 and initializer 0x195A40.

See ../docs/halo2-clan-matchmaking-evidence.md. Membership, serialization and
unanimous result arbitration are replacement service policies.
"""
from dataclasses import dataclass
from .native_database import run as database

from .playlist_skill import TABLES, Skill
from .native_matchmaking import value


@dataclass(frozen=True)
class RatingClan:
    skill: int
    place: int
    xp: int
    active: bool = True


def clan_delta(clans,selected,tables=TABLES):
    return value('rank_clan',clans=tuple(clans),selected=selected,tables=tables)


class ClanSkills:
    """Typed IO to Rust-owned skill schemas, snapshots and migrations."""
    def __init__(self, db):
        self.db = db
        database(db, 'clan_schema')

    def snapshot(self, clan, playlist, configuration):
        return Skill(**database(self.db, 'clan_snapshot', owner=clan,
                               playlist=playlist, configuration=configuration))
