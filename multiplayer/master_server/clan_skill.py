"""Clan-owned hopper XP: retail 0x74720/741F0 and initializer 0x195A40.

See ../docs/halo2-clan-matchmaking-evidence.md. Membership, serialization and
unanimous result arbitration are replacement service policies.
"""
from dataclasses import dataclass

from .playlist_skill import (POLICY, TABLE_DIGEST, TABLES, MAX_XP, Skill,
                             pair_delta, divide, raw_rank)
from .protocol import require


@dataclass(frozen=True)
class RatingClan:
    skill: int
    place: int
    xp: int
    active: bool = True


def clan_delta(clans, selected, tables=TABLES):
    """0x74720: average other active clan entries, including ties, then bound."""
    own = clans[selected]
    comparisons = [pair_delta(own.skill, other.skill, own.place, other.place, tables)
                   for i, other in enumerate(clans)
                   if i != selected and other.active and other.skill != -1]
    delta = divide(sum(comparisons), len(comparisons)) if comparisons else 0
    delta = max(-own.xp, min(MAX_XP-own.xp, delta))
    maximum, minimum = tables["delta_bounds"]
    return max(minimum, min(maximum, delta))


class ClanSkills:
    def __init__(self, db):
        self.db = db
        db.executescript("""
        CREATE TABLE IF NOT EXISTS clan_playlist_skill(
          clan TEXT NOT NULL REFERENCES clans(id) ON DELETE CASCADE,
          playlist TEXT NOT NULL, configuration TEXT NOT NULL,
          raw_rank INTEGER NOT NULL CHECK(raw_rank BETWEEN 0 AND 127),
          xp INTEGER NOT NULL CHECK(xp BETWEEN 0 AND 1073741823),
          PRIMARY KEY(clan,playlist));
        CREATE TABLE IF NOT EXISTS match_result_clan_skill(
          match TEXT NOT NULL, team INTEGER NOT NULL, clan TEXT NOT NULL,
          configuration TEXT NOT NULL, table_digest TEXT NOT NULL,
          effective_rank INTEGER NOT NULL, previous_rank INTEGER NOT NULL,
          previous_xp INTEGER NOT NULL, delta INTEGER NOT NULL,
          raw_rank INTEGER NOT NULL, xp INTEGER NOT NULL,
          PRIMARY KEY(match,team),
          FOREIGN KEY(match,team) REFERENCES match_result_teams(match,team));
        """)

    def snapshot(self, clan, playlist, configuration):
        require(configuration == POLICY, "Unknown clan rank configuration", 409)
        db = self.db
        with db:
            row = db.execute("SELECT raw_rank,xp,configuration FROM clan_playlist_skill WHERE clan=? AND playlist=?",
                             (clan, playlist)).fetchone()
            if row is None:
                db.execute("INSERT INTO clan_playlist_skill VALUES(?,?,?,0,0)", (clan, playlist, configuration))
                return Skill(0, 0, configuration)
            require(row[2] == configuration, "This clan playlist requires an explicit skill migration", 409)
            return Skill(*row)

    def commit(self, room):
        """Enclosed by the immutable match ledger's single transaction."""
        if not room.clan_configuration:
            return
        db, playlist = self.db, room.proposal.playlist
        teams = sorted(room.clans)
        ratings = []
        for team in teams:
            clan = room.clans[team]
            skill = room.clan_skills[clan]
            current = db.execute("SELECT raw_rank,xp,configuration FROM clan_playlist_skill WHERE clan=? AND playlist=?",
                                 (clan, playlist)).fetchone()
            require(current is not None and tuple(current) == (skill.raw, skill.xp, skill.configuration),
                    "Clan skill changed during this match", 409)
            ratings.append(RatingClan(skill.raw, room.result[0xcb0+team*0x18+1], skill.xp))
        for index, team in enumerate(teams):
            clan = room.clans[team]
            previous = room.clan_skills[clan]
            delta = clan_delta(ratings, index)
            xp = previous.xp + delta
            rank = raw_rank(xp, previous.raw)
            db.execute("UPDATE clan_playlist_skill SET raw_rank=?,xp=? WHERE clan=? AND playlist=?",
                       (rank, xp, clan, playlist))
            # Historical owners survive clan dissolution; only the live skill
            # row has a cascading foreign key to the current clan registry.
            db.execute("INSERT INTO match_result_clan_skill VALUES(?,?,?,?,?,?,?,?,?,?,?)",
                       (room.id, team, clan, previous.configuration, TABLE_DIGEST, previous.raw,
                        previous.raw, previous.xp, delta, rank, xp))
