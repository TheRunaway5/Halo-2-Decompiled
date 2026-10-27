"""Authenticated current Halo appearance; historical PCR appearance stays immutable."""
import re
from collections import OrderedDict
from time import monotonic
from aiohttp import web
from .protocol import fields, identifier, require
from .player_statistics import initialize, enqueue, totals
from .friend_tags import tag

FORMAT = "halo2-profile-appearance-v1"


def appearance(value):
    require(type(value) is str and re.fullmatch(r"[0-9a-f]{18}", value),
            "Supply the exact nine-byte Halo profile appearance")
    raw = bytes.fromhex(value)
    require(raw[4] in (0, 1), "Player species must be Spartan or Elite")
    return raw


def decoded(raw):
    if raw is None:
        return None
    return dict(format=FORMAT, bytes=raw.hex(),
                species="elite" if raw[4] == 1 else "spartan",
                colors=[n if n < 128 else n-256 for n in raw[:4]],
                emblem=dict(foreground=raw[5], background=raw[6], flags=raw[7], reserved=raw[8]))


class PlayerProfiles:
    def __init__(self, directory):
        self.directory, self.db = directory, directory.store.db
        self._statistics = OrderedDict()
        initialize(self.db)
        self.db.executescript("""
        CREATE TABLE IF NOT EXISTS player_customization(
          player TEXT PRIMARY KEY REFERENCES players(id),
          appearance BLOB NOT NULL CHECK(length(appearance)=9),
          service_tag TEXT NOT NULL DEFAULT '', updated_at INTEGER NOT NULL,
          revision INTEGER NOT NULL);
        """)

    def view(self, who):
        row = self.db.execute("SELECT p.screen_name,p.joined_at,c.appearance,c.service_tag,c.updated_at,c.revision "
            "FROM player_profiles p LEFT JOIN player_customization c ON c.player=p.player WHERE p.player=?", (who,)).fetchone()
        require(row is not None, "Player profile not found", 404)
        return dict(screen_name=row[0], friend_tag=tag(self.db,who), joined_at=row[1], appearance=decoded(row[2]),
                    service_tag=row[3], updated_at=row[4], revision=row[5] or 0,
                    statistics=self.statistics(who))

    def statistics(self, who):
        # Accepted results and customization share this durable outbox revision.
        # Heartbeats and public history pages must not rescan every past match.
        row = self.db.execute("SELECT revision FROM eos_profile_outbox WHERE player=?", (who,)).fetchone()
        revision = row[0] if row else None
        cached = self._statistics.get(who)
        now = monotonic()
        if cached is None or cached[0] != revision or cached[1] <= now:
            cached = (revision, now + 300, totals(self.db, who))
            self._statistics[who] = cached
        self._statistics.move_to_end(who)
        while len(self._statistics) > 1024:
            self._statistics.popitem(last=False)
        return dict(cached[2])

    def save(self, who, raw, tag=None):
        previous = self.db.execute("SELECT appearance,service_tag FROM player_customization WHERE player=?", (who,)).fetchone()
        tag = (previous[1] if previous else "") if tag is None else tag
        if previous and (bytes(previous[0]), previous[1]) == (raw, tag):
            return
        self.db.execute("INSERT INTO player_customization VALUES(?,?,?,?,1) ON CONFLICT(player) DO UPDATE SET "
                        "appearance=excluded.appearance,service_tag=excluded.service_tag,updated_at=excluded.updated_at,"
                        "revision=player_customization.revision+1", (who,raw,tag,self.directory.store.now()))
        enqueue(self.db,who)

    async def get(self, request):
        _, who = self.directory.clans.session(request)
        return web.json_response(self.view(who))

    async def update(self, request):
        _, who = self.directory.clans.session(request)
        body = await self.directory.body(request)
        fields(body, ("appearance",), ("service_tag",))
        raw = appearance(body["appearance"])
        tag = body.get("service_tag")
        require(tag is None or type(tag) is str and bool(re.fullmatch(r"[A-Z0-9]{0,4}",tag)),
                "Service tag must contain up to four uppercase letters or digits")
        with self.db:
            self.save(who,raw,tag)
        return web.json_response(self.view(who))

    async def public(self, request):
        public = identifier(request.match_info["player"])
        row = self.db.execute("SELECT player FROM history_players WHERE public_id=?",(public,)).fetchone()
        require(row is not None, "Player profile not found",404)
        return web.json_response(dict(id=public,**self.view(row[0])))

    def routes(self):
        return [web.get("/v1/account/profile",self.get),web.put("/v1/account/profile",self.update),
                web.get("/v1/history/players/{player}/profile",self.public)]
