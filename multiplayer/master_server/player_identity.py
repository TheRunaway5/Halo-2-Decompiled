"""Stable private identities shared by queue and social sessions."""
from dataclasses import dataclass
import re
import uuid

from aiohttp import web
from .protocol import require

VERIFIED_ACCOUNT_NAME = web.RequestKey("verified_account_name", str)
VERIFIED_DISCORD_USERNAME = VERIFIED_ACCOUNT_NAME  # Existing callers retain their API.


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
    CREATE TABLE IF NOT EXISTS player_identities(provider TEXT NOT NULL,
      subject TEXT NOT NULL, player TEXT NOT NULL REFERENCES players(id),
      PRIMARY KEY(provider,subject), UNIQUE(player,provider));
    INSERT OR IGNORE INTO player_identities SELECT provider,subject,id FROM players;
    """)
    if "steam_name" not in {r[1] for r in db.execute("PRAGMA table_info(player_profiles)")}:
        db.execute("ALTER TABLE player_profiles ADD COLUMN steam_name TEXT NOT NULL DEFAULT ''")


def player(db, owner, name, username=""):
    # Only the upstream verifier may supply a username. Local screen names are
    # deliberately nonunique and cannot be used for identity or authorization.
    require(len(owner) == 2 and all(type(v) is str and v for v in owner), "Verified account identity required", 401)
    if owner[0] != "steam":
        require(not username or re.fullmatch(r"[a-z0-9_.]{2,32}", username), "Invalid verified Discord username", 403)
    else:
        require(type(username) is str and len(username.encode("utf-8")) <= 128 and
                not any(ord(c) < 32 or ord(c) == 127 for c in username), "Invalid verified Steam name", 403)
    with db:
        row = db.execute("SELECT player FROM player_identities WHERE provider=? AND subject=?", owner).fetchone()
        if row:
            value = row[0]
        else:
            require(db.execute("SELECT count(*) FROM players").fetchone()[0] < 1000, "Player capacity reached", 503)
            value = str(uuid.uuid4())
            db.execute("INSERT INTO players VALUES(?,?,?)", (value, *owner))
            db.execute("INSERT INTO player_identities VALUES(?,?,?)", (*owner, value))
        discord_name = username if owner[0] == "discord" else ""
        steam_name = username if owner[0] == "steam" else ""
        if discord_name:
            db.execute("UPDATE player_profiles SET discord_username='' WHERE discord_username=? AND player!=?", (username, value))
        db.execute("INSERT INTO player_profiles(player,screen_name,discord_username,steam_name) VALUES(?,?,?,?) "
                   "ON CONFLICT(player) DO UPDATE SET screen_name=excluded.screen_name, "
                   "discord_username=CASE WHEN excluded.discord_username!='' THEN excluded.discord_username ELSE player_profiles.discord_username END, "
                   "steam_name=CASE WHEN excluded.steam_name!='' THEN excluded.steam_name ELSE player_profiles.steam_name END",
                   (value, name, discord_name, steam_name))
    return value
