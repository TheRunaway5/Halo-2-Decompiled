"""Stable private identities shared by queue and social sessions."""
from dataclasses import dataclass
import re
import uuid

from aiohttp import web
from .protocol import require

VERIFIED_DISCORD_USERNAME = web.RequestKey("verified_discord_username", str)


@dataclass(frozen=True)
class VerifiedOwner:
    provider: str
    subject: str
    username: str


def initialize(db):
    db.executescript("""
    CREATE TABLE IF NOT EXISTS players(id TEXT PRIMARY KEY, provider TEXT NOT NULL,
      subject TEXT NOT NULL, UNIQUE(provider,subject));
    CREATE TABLE IF NOT EXISTS player_profiles(player TEXT PRIMARY KEY REFERENCES players(id),
      screen_name TEXT NOT NULL, discord_username TEXT NOT NULL DEFAULT '');
    CREATE UNIQUE INDEX IF NOT EXISTS discord_usernames ON player_profiles(discord_username)
      WHERE discord_username != '';
    """)


def player(db, owner, name, username=""):
    # Only the upstream verifier may supply a username. Local screen names are
    # deliberately nonunique and cannot be used for identity or authorization.
    require(not username or re.fullmatch(r"[a-z0-9_.]{2,32}", username), "Invalid verified Discord username", 403)
    with db:
        row = db.execute("SELECT id FROM players WHERE provider=? AND subject=?", owner).fetchone()
        if row:
            value = row[0]
        else:
            require(db.execute("SELECT count(*) FROM players").fetchone()[0] < 1000, "Player capacity reached", 503)
            value = str(uuid.uuid4())
            db.execute("INSERT INTO players VALUES(?,?,?)", (value, *owner))
        if username:
            db.execute("UPDATE player_profiles SET discord_username='' WHERE discord_username=? AND player!=?", (username, value))
        db.execute("INSERT INTO player_profiles VALUES(?,?,?) ON CONFLICT(player) DO UPDATE SET screen_name=excluded.screen_name, discord_username=CASE WHEN excluded.discord_username!='' THEN excluded.discord_username ELSE player_profiles.discord_username END", (value, name, username))
    return value
