"""Private, account-owned Discord block preferences for the native voice relays.

These are client-supplied preferences, not an independently verified Discord
relationship service. No block list or identity resolver is publicly readable.
The last successful full snapshot survives SDK disconnects and server restarts.
"""
import re
from aiohttp import web
from .protocol import fields, require


class VoiceBlocks:
    def __init__(self, directory):
        self.directory, self.db = directory, directory.store.db
        self.db.execute("""CREATE TABLE IF NOT EXISTS voice_blocks(
            player TEXT NOT NULL REFERENCES players(id), subject TEXT NOT NULL,
            PRIMARY KEY(player,subject))""")
        self.blocks, self.subjects = {}, {}
        for who, subject in self.db.execute("SELECT player,subject FROM voice_blocks"):
            self.blocks.setdefault(who, set()).add(subject)

    def subject(self, who):
        if who not in self.subjects:
            row = self.db.execute("SELECT subject FROM players WHERE id=? AND provider='discord' UNION ALL "
                                  "SELECT subject FROM player_identities WHERE player=? AND provider='discord' LIMIT 1", (who, who)).fetchone()
            if row is not None:
                self.subjects[who] = row[0]
            return row[0] if row else None
        return self.subjects[who]

    def allowed(self, a, b):
        friends = getattr(self.directory,"friends",None)
        if friends and friends.blocked(a,b):
            return False
        sa, sb = self.subject(a), self.subject(b)
        known = all(self.db.execute("SELECT 1 FROM players WHERE id=?", (who,)).fetchone() for who in (a, b))
        return (known and sb not in self.blocks.get(a, ()) and sa not in self.blocks.get(b, ()))

    async def update(self, request):
        body = await self.directory.body(request, limit=48 * 1024)
        fields(body, ("blocked",))
        rows = body["blocked"]
        require(type(rows) is list and len(rows) <= 2000, "Invalid voice block list")
        require(all(type(v) is str and re.fullmatch(r"[0-9]{17,20}", v) for v in rows),
                "Invalid voice block list")
        require(len(set(rows)) == len(rows), "Invalid voice block list")
        provider, subject = await self.directory.owner(request)
        require(provider in ("discord", "steam"), "Account identity required", 403)
        row = self.db.execute("SELECT id FROM players WHERE provider=? AND subject=? UNION ALL "
                              "SELECT player FROM player_identities WHERE provider=? AND subject=? LIMIT 1",
                              (provider, subject, provider, subject)).fetchone()
        require(row is not None, "Connect to ONLINE before syncing voice preferences", 409)
        who = row[0]
        # No await between the authenticated replacement and relay notification.
        discord_subject = subject if provider == "discord" else self.subject(who)
        values = set(rows) - {discord_subject}
        total = self.db.execute("SELECT count(*) FROM voice_blocks").fetchone()[0]
        require(total - len(self.blocks.get(who, ())) + len(values) <= 100000,
                "Voice preference capacity reached", 503)
        with self.db:
            self.db.execute("DELETE FROM voice_blocks WHERE player=?", (who,))
            self.db.executemany("INSERT INTO voice_blocks VALUES(?,?)", ((who, s) for s in values))
        self.blocks[who] = values
        if discord_subject is not None:
            self.subjects[who] = discord_subject
        # Constant response: unknown targets must not become an account lookup.
        return web.json_response({"state": "updated"})
