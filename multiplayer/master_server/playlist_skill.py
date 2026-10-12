"""Retail launch XP arithmetic and private, versioned playlist skill storage.

0x746B0/74800/74970 from the pinned retail executable. The literal initializer
tables are included; downloaded configurations and later Live policies are not
inferred. This native service owns arbitration and never accepts client XP.
"""
from dataclasses import dataclass
import hashlib
import json
from pathlib import Path

from .protocol import integer, require

POLICY = "retail-launch-2004"
MAX_XP = 0x3fffffff
_raw = (Path(__file__).parent / "data/retail-launch-2004-rank.json").read_bytes()
TABLE_DIGEST = "5b5760a487d6af59da885edb89f2d7d7e76df5fa0d2379cb7a43ff9dfa3e55bd"
require(hashlib.sha256(_raw).hexdigest() == TABLE_DIGEST, "Unverified retail rank configuration", 500)
TABLES = json.loads(_raw)["tables"]


def divide(value, denominator):
    """Signed x86 IDIV truncates toward zero, including negative averages."""
    return (1 if value >= 0 else -1) * (abs(value) // denominator)


def pair_delta(own, other, place, other_place, tables=TABLES):
    integer(own, 0, 127)
    integer(other, 0, 127)
    if place != -1 and (other_place == -1 or place < other_place):
        outcome = "win"
    elif other_place != -1 and (place == -1 or other_place < place):
        outcome = "loss"
    else:
        return 0
    return (divide(tables[outcome + "_exchange"][own - other + 127] *
                   tables[outcome + "_percent"][own], 100) + tables[outcome + "_offsets"][own])


@dataclass(frozen=True)
class RatingPlayer:
    skill: int
    place: int
    team: int
    xp: int = 5000
    key: int = 3
    flags: int = 0
    active: bool = True


def player_delta(players, selected, *, key=3, team_places=None, tables=TABLES):
    """0x74800: exclude self/teammates, retain ties in the divisor, bound XP."""
    own = players[selected]
    place = own.place if team_places is None else team_places[own.team]
    comparisons = [pair_delta(own.skill, other.skill, place,
                             other.place if team_places is None else team_places[other.team], tables)
                   for i, other in enumerate(players)
                   if other.key == key and other.active and other.team != -1 and
                   not other.flags & 3 and other.skill != -1 and i != selected and other.team != own.team]
    delta = divide(sum(comparisons), len(comparisons)) if comparisons else 0
    if own.xp + delta < 0:
        delta = -own.xp
    if own.xp + delta > MAX_XP:
        delta = MAX_XP - own.xp
    maximum, minimum = tables["delta_bounds"]
    return minimum if delta < minimum else maximum if delta > maximum else delta


def raw_rank(xp, previous, tables=TABLES):
    """0x74970: one-rank demotion hysteresis, with a strict midpoint test."""
    integer(xp, 0, MAX_XP)
    integer(previous, 0, 127)
    thresholds = tables["xp_thresholds"]
    for i in range(1, 128):
        if xp < thresholds[i]:
            candidate = i - 1
            return previous if candidate == previous - 1 and xp > divide(
                thresholds[candidate] + thresholds[i], 2) else candidate
    return 127


def display_level(raw):
    # Original wigl ranges 0..48 are singletons; 49..127 share the halo frame.
    return min(integer(raw, 0, 127), 49) + 1


@dataclass(frozen=True)
class Skill:
    raw: int
    xp: int
    configuration: str


class PlaylistSkills:
    def __init__(self, db):
        self.db = db
        db.executescript("""
        CREATE TABLE IF NOT EXISTS playlist_skill(
          player TEXT NOT NULL REFERENCES players(id), playlist TEXT NOT NULL,
          configuration TEXT NOT NULL, raw_rank INTEGER NOT NULL CHECK(raw_rank BETWEEN 0 AND 127),
          xp INTEGER NOT NULL CHECK(xp BETWEEN 0 AND 1073741823),
          PRIMARY KEY(player,playlist));
        CREATE TABLE IF NOT EXISTS match_result_skill(
          match TEXT NOT NULL, player TEXT NOT NULL, configuration TEXT NOT NULL,
          table_digest TEXT NOT NULL, effective_rank INTEGER NOT NULL,
          previous_rank INTEGER NOT NULL, previous_xp INTEGER NOT NULL,
          delta INTEGER NOT NULL, raw_rank INTEGER NOT NULL, xp INTEGER NOT NULL,
          PRIMARY KEY(match,player),
          FOREIGN KEY(match,player) REFERENCES match_result_players(match,player));
        """)

    def snapshot(self, player, playlist, configuration):
        require(configuration == POLICY, "Unknown rank configuration", 409)
        db = self.db
        with db:
            row = db.execute("SELECT raw_rank,xp,configuration FROM playlist_skill WHERE player=? AND playlist=?",
                             (player, playlist)).fetchone()
            if row is None:
                legacy = db.execute("SELECT rank FROM playlist_ranks WHERE player=? AND playlist=?",
                                    (player, playlist)).fetchone()
                require(legacy is None or legacy[0] == 1,
                        "This playlist requires an explicit skill migration", 409)
                db.execute("INSERT INTO playlist_skill VALUES(?,?,?,0,0)", (player, playlist, configuration))
                db.execute("INSERT OR IGNORE INTO playlist_ranks VALUES(?,?,1)", (player, playlist))
                return Skill(0, 0, configuration)
            require(row[2] == configuration, "This playlist requires an explicit skill migration", 409)
            return Skill(*row)

    def commit(self, room):
        """Called within the same transaction as the immutable result ledger."""
        if not room.skill_configuration:
            return
        db, playlist, raw = self.db, room.proposal.playlist, room.result
        players = []
        team_places = {team: raw[0xcb0 + team*0x18 + 1] for _, _, team in room.players} if room.team_mode else None
        for slot, (member, _, team) in enumerate(room.players):
            skill = room.skills[member]
            current = db.execute("SELECT raw_rank,xp,configuration FROM playlist_skill WHERE player=? AND playlist=?",
                                 (member, playlist)).fetchone()
            require(current is not None and tuple(current) == (skill.raw, skill.xp, skill.configuration),
                    "Playlist skill changed during this match", 409)
            # One signed-in native player per machine. Search-party adjustments
            # do not overwrite persistent raw rank or XP used by this policy.
            players.append(RatingPlayer(skill.raw, raw[0x370+slot*0x94+0x90],
                                        team if room.team_mode else slot, skill.xp))
        for slot, (member, _, _) in enumerate(room.players):
            previous = room.skills[member]
            delta = player_delta(players, slot, team_places=team_places)
            xp = previous.xp + delta
            rank = raw_rank(xp, previous.raw)
            db.execute("UPDATE playlist_skill SET raw_rank=?,xp=? WHERE player=? AND playlist=?",
                       (rank, xp, member, playlist))
            db.execute("UPDATE playlist_ranks SET rank=? WHERE player=? AND playlist=?",
                       (display_level(rank), member, playlist))
            db.execute("INSERT INTO match_result_skill VALUES(?,?,?,?,?,?,?,?,?,?)",
                       (room.id, member, previous.configuration, TABLE_DIGEST, players[slot].skill,
                        previous.raw, previous.xp, delta, rank, xp))
