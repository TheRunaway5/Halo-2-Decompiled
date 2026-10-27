"""Public friend tags. Authentication and ownership always use private player IDs."""
import re
import secrets
import sqlite3
import unicodedata

from .protocol import require, text

DIGITS = 8


def initialize(db):
    # Schema migration deliberately does not allocate tags to dormant accounts.
    db.execute("""CREATE TABLE IF NOT EXISTS player_friend_tags(
        player TEXT PRIMARY KEY REFERENCES players(id),
        discriminator TEXT NOT NULL UNIQUE CHECK(length(discriminator)=8 AND
          discriminator NOT GLOB '*[^0-9]*'))""")


def assign(db, who):
    """Caller owns the authenticated identity transaction; preserve on rename."""
    if db.execute("SELECT 1 FROM player_friend_tags WHERE player=?", (who,)).fetchone():
        return
    for _ in range(128):
        suffix = f"{secrets.randbelow(10 ** DIGITS):0{DIGITS}d}"
        try:
            db.execute("INSERT INTO player_friend_tags VALUES(?,?)", (who, suffix))
            return
        except sqlite3.IntegrityError:
            if db.execute("SELECT 1 FROM player_friend_tags WHERE player=?", (who,)).fetchone():
                return
            if not db.execute("SELECT 1 FROM player_friend_tags WHERE discriminator=?", (suffix,)).fetchone():
                raise
    require(False, "Friend tag allocation is busy; reconnect shortly", 503)


def tag(db, who):
    row = db.execute("SELECT p.screen_name,t.discriminator FROM player_profiles p "
                     "JOIN player_friend_tags t ON t.player=p.player WHERE p.player=?", (who,)).fetchone()
    return f"{row[0]}#{row[1]}" if row else None


def fold(value):
    return unicodedata.normalize("NFC", value).casefold()


def resolve(db, value):
    value = text(value, 73)
    name, separator, suffix = value.rpartition("#")
    require(separator and name and re.fullmatch(r"[0-9]{8}", suffix),
            "Enter a friend tag such as EmkMage#12345678")
    row = db.execute("SELECT t.player,p.screen_name FROM player_friend_tags t "
                     "JOIN player_profiles p ON p.player=t.player WHERE t.discriminator=?", (suffix,)).fetchone()
    # The old name stops resolving after a rename, but the suffix never changes.
    require(row and fold(row[1]) == fold(name), "Player not found; check their current friend tag", 404)
    return row[0]
