"""Persistent social clans, authorized exclusively by verified Discord identity.

Original role/lifecycle evidence: ../docs/halo2-clan-evidence.md.
HTTP sessions, invitation expiry, text broadcasts and quotas are native policy.
"""
from dataclasses import dataclass
import hashlib
import json
import re
import secrets
import time
import uuid

from aiohttp import web
from .protocol import fields, identifier, integer, require, text, canonical
from .player_identity import initialize, player, VERIFIED_DISCORD_USERNAME


@dataclass
class Session:
    player: str
    expires: float
    seen: float


class ClanApi:
    def __init__(self, directory, *, clock=time.monotonic, wall=time.time):
        self.directory, self.clock, self.wall = directory, clock, wall
        self.db = directory.store.db
        self.sessions = {}
        initialize(self.db)
        self.db.executescript("""
        CREATE TABLE IF NOT EXISTS clans(id TEXT PRIMARY KEY, name TEXT NOT NULL,
          folded_name TEXT NOT NULL UNIQUE);
        CREATE TABLE IF NOT EXISTS clan_members(player TEXT PRIMARY KEY REFERENCES players(id),
          clan TEXT NOT NULL REFERENCES clans(id) ON DELETE CASCADE,
          role INTEGER NOT NULL CHECK(role BETWEEN 0 AND 3), joined INTEGER NOT NULL);
        CREATE INDEX IF NOT EXISTS clan_roster ON clan_members(clan);
        CREATE TABLE IF NOT EXISTS clan_invites(id TEXT PRIMARY KEY,
          clan TEXT NOT NULL REFERENCES clans(id) ON DELETE CASCADE,
          target TEXT NOT NULL REFERENCES players(id), inviter TEXT NOT NULL REFERENCES players(id),
          expires INTEGER NOT NULL, UNIQUE(clan,target));
        CREATE TABLE IF NOT EXISTS clan_blocks(player TEXT NOT NULL REFERENCES players(id),
          clan TEXT NOT NULL REFERENCES clans(id) ON DELETE CASCADE, PRIMARY KEY(player,clan));
        CREATE TABLE IF NOT EXISTS clan_messages(id TEXT PRIMARY KEY,
          clan TEXT NOT NULL REFERENCES clans(id) ON DELETE CASCADE,
          author TEXT NOT NULL REFERENCES players(id), name TEXT NOT NULL,
          body TEXT NOT NULL, created INTEGER NOT NULL);
        CREATE TABLE IF NOT EXISTS clan_requests(player TEXT NOT NULL REFERENCES players(id),
          request TEXT NOT NULL, payload TEXT NOT NULL, expires INTEGER NOT NULL,
          PRIMARY KEY(player,request));
        """)

    def cleanup(self):
        self.sessions = {k: s for k, s in self.sessions.items() if s.expires > self.clock()}
        with self.db:
            now = int(self.wall())
            self.db.execute("DELETE FROM clan_invites WHERE expires<=?", (now,))
            self.db.execute("DELETE FROM clan_requests WHERE expires<=?", (now,))
            self.db.execute("DELETE FROM clan_messages WHERE created<?", (now - 30 * 86400,))

    def session(self, request):
        self.cleanup()
        header = request.headers.get("Authorization", "")
        require(header.startswith("Bearer ") and len(header) <= 256, "ONLINE session required", 401)
        key = hashlib.sha256(header[7:].encode()).hexdigest()
        session = self.sessions.get(key)
        require(session is not None, "ONLINE session expired; reconnect", 401)
        self.directory.rate("social:" + session.player, 120)
        session.seen = self.clock()
        return key, session.player

    async def login(self, request):
        body = await self.directory.body(request)
        fields(body, ("screen_name",))
        name = text(body["screen_name"])
        owner = await self.directory.owner(request)
        self.cleanup()
        who = player(self.db, owner, name, request.get(VERIFIED_DISCORD_USERNAME, ""), now=self.directory.store.now())
        with self.db:
            self.directory.matchmaking.results.history.identity(who)
        self.sessions = {k: s for k, s in self.sessions.items() if s.player != who}
        require(len(self.sessions) < 1000, "ONLINE session capacity reached", 503)
        token = secrets.token_urlsafe(32)
        self.sessions[hashlib.sha256(token.encode()).hexdigest()] = Session(who, self.clock() + 900, self.clock())
        return web.json_response({"token": token, "expires_in": 900}, status=201)

    async def logout(self, request):
        key, _ = self.session(request)
        del self.sessions[key]
        return web.json_response({"state": "signed_out"})

    def member(self, who):
        return self.db.execute("SELECT m.clan,m.role,c.name FROM clan_members m JOIN clans c ON c.id=m.clan WHERE m.player=?", (who,)).fetchone()

    def view(self, who, presence=False):
        member = self.member(who)
        clan = None
        if member:
            clan_id, role, name = member
            members = [{"id": r[0], "name": r[1], "role": r[2], "self": r[0] == who}
                       for r in self.db.execute("SELECT m.player,p.screen_name,m.role FROM clan_members m JOIN player_profiles p ON p.player=m.player WHERE m.clan=? ORDER BY m.role DESC,m.joined,m.player", (clan_id,))]
            if presence:
                from .player_presence import observed
                states = observed(self.directory)
                for member in members:
                    member["presence"] = states.get(member["id"], "offline")
            outgoing = [{"id": r[0], "name": r[1]} for r in self.db.execute("SELECT i.id,p.screen_name FROM clan_invites i JOIN player_profiles p ON p.player=i.target WHERE i.clan=? AND (i.inviter=? OR ? >= 2) ORDER BY i.expires,i.id", (clan_id, who, role))]
            messages = [{"id": r[0], "name": r[1], "body": r[2]} for r in self.db.execute("SELECT id,name,body FROM clan_messages WHERE clan=? ORDER BY created DESC,rowid DESC LIMIT 20", (clan_id,))]
            clan = {"id": clan_id, "name": name, "role": role, "members": members,
                    "outgoing": outgoing, "messages": messages}
        incoming = [{"id": r[0], "name": r[1]} for r in self.db.execute("SELECT i.id,c.name FROM clan_invites i JOIN clans c ON c.id=i.clan WHERE i.target=? ORDER BY i.expires,i.id LIMIT 100", (who,))]
        return {"clan": clan, "invitations": incoming}

    async def get(self, request):
        _, who = self.session(request)
        return web.json_response(self.view(who, request.query.get("presence") == "1"))

    async def command(self, request):
        body = await self.directory.body(request)
        fields(body, ("request_id", "action"), ("name", "username", "target", "role", "confirmed", "message"))
        rid, action = identifier(body["request_id"]), text(body["action"])
        # No await between authorization, replay check and transaction commit.
        _, who = self.session(request)
        payload = canonical(body).decode()
        old = self.db.execute("SELECT payload FROM clan_requests WHERE player=? AND request=?", (who, rid)).fetchone()
        if old:
            require(old[0] == payload, "Command ID reused with different fields", 409)
            return web.json_response(self.view(who, request.query.get("presence") == "1"))
        require(self.db.execute("SELECT count(*) FROM clan_requests WHERE player=?", (who,)).fetchone()[0] < 256,
                "Clan command quota reached; retry tomorrow", 429)
        with self.db:
            self.apply(who, action, body)
            self.db.execute("INSERT INTO clan_requests VALUES(?,?,?,?)", (who, rid, payload, int(self.wall()) + 86400))
        return web.json_response(self.view(who, request.query.get("presence") == "1"))

    def apply(self, who, action, body):
        schemas = {"create": ("name",), "invite": ("username",), "accept": ("target",),
                   "decline": ("target",), "block": ("target",), "cancel": ("target",),
                   "remove": ("target",), "role": ("target", "role", "confirmed"),
                   "leave": (), "broadcast": ("message",)}
        require(action in schemas, "Unknown clan action")
        fields(body, ("request_id", "action", *schemas[action]))
        m = self.member(who)
        now = int(self.wall())
        if action == "create":
            require(m is None, "Leave your current clan first", 409)
            name = text(body["name"], 15).strip()
            require(bool(re.fullmatch(r"[A-Za-z0-9]+(?:[ A-Za-z0-9]*[A-Za-z0-9])?", name)), "Clan names use letters, numbers and spaces")
            require(not self.db.execute("SELECT 1 FROM clans WHERE folded_name=?", (name.lower(),)).fetchone(), "Clan name already exists", 409)
            require(self.db.execute("SELECT count(*) FROM clans").fetchone()[0] < 1000, "Clan capacity reached", 503)
            cid = str(uuid.uuid4())
            self.db.execute("INSERT INTO clans VALUES(?,?,?)", (cid, name, name.lower()))
            self.db.execute("INSERT INTO clan_members VALUES(?,?,3,?)", (who, cid, now))
            self.db.execute("DELETE FROM clan_invites WHERE target=?", (who,))
            return
        if action in ("accept", "decline", "block"):
            iid = identifier(body["target"])
            invite = self.db.execute("SELECT clan FROM clan_invites WHERE id=? AND target=?", (iid, who)).fetchone()
            require(invite is not None, "Clan invitation no longer available", 404)
            cid = invite[0]
            if action == "accept":
                require(m is None, "Leave your current clan first", 409)
                require(self.db.execute("SELECT count(*) FROM clan_members WHERE clan=?", (cid,)).fetchone()[0] < 100, "Clan is full", 409)
                self.db.execute("INSERT INTO clan_members VALUES(?,?,1,?)", (who, cid, now))
                self.db.execute("DELETE FROM clan_invites WHERE target=?", (who,))
            else:
                self.db.execute("DELETE FROM clan_invites WHERE id=?", (iid,))
                if action == "block":
                    self.db.execute("INSERT OR IGNORE INTO clan_blocks VALUES(?,?)", (who, cid))
            return
        require(m is not None, "You are not in a clan", 409)
        cid, role, _ = m
        if action == "invite":
            require(role >= 1, "Member role required to recruit", 403)
            username = text(body["username"], 32).lower()
            require(bool(re.fullmatch(r"[a-z0-9_.]{2,32}", username)), "Enter a Discord username")
            row = self.db.execute("SELECT player FROM player_profiles WHERE discord_username=?", (username,)).fetchone()
            require(row is not None, "Player must sign into ONLINE before being invited", 404)
            target = row[0]
            require(self.member(target) is None, "Player already belongs to a clan", 409)
            require(not self.db.execute("SELECT 1 FROM clan_blocks WHERE player=? AND clan=?", (target, cid)).fetchone(), "Player is not accepting invitations", 403)
            require(self.db.execute("SELECT count(*) FROM clan_members WHERE clan=?", (cid,)).fetchone()[0] < 100, "Clan is full", 409)
            require(self.db.execute("SELECT count(*) FROM clan_invites WHERE clan=?", (cid,)).fetchone()[0] < 100, "Clan invitation limit reached", 429)
            require(self.db.execute("SELECT count(*) FROM clan_invites WHERE target=?", (target,)).fetchone()[0] < 100, "Player invitation limit reached", 429)
            self.db.execute("INSERT INTO clan_invites VALUES(?,?,?,?,?) ON CONFLICT(clan,target) DO NOTHING", (str(uuid.uuid4()), cid, target, who, now + 7 * 86400))
        elif action == "cancel":
            target = identifier(body["target"])
            row = self.db.execute("SELECT inviter FROM clan_invites WHERE id=? AND clan=?", (target, cid)).fetchone()
            require(row is not None, "Invitation no longer available", 404)
            require(row[0] == who or role >= 2, "Cannot cancel this invitation", 403)
            self.db.execute("DELETE FROM clan_invites WHERE id=?", (target,))
        elif action in ("role", "remove"):
            target = identifier(body["target"])
            row = self.db.execute("SELECT role FROM clan_members WHERE clan=? AND player=?", (cid, target)).fetchone()
            require(row is not None, "Member no longer available", 404)
            require(role >= 2 and role > row[0], "Cannot change a member of equal or higher role", 403)
            if action == "role":
                rank = integer(body["role"], 0, 3)
                require(rank <= role, "Cannot grant a higher role than your own", 403)
                require(type(body["confirmed"]) is bool and (rank != 3 or body["confirmed"]), "Confirm irreversible Overlord promotion", 409)
                self.db.execute("UPDATE clan_members SET role=? WHERE player=?", (rank, target))
            else:
                self.directory.matchmaking.clan_membership_changing(target)
                self.db.execute("DELETE FROM clan_members WHERE player=?", (target,))
        elif action == "leave":
            count = self.db.execute("SELECT count(*) FROM clan_members WHERE clan=?", (cid,)).fetchone()[0]
            leaders = self.db.execute("SELECT count(*) FROM clan_members WHERE clan=? AND role=3", (cid,)).fetchone()[0]
            require(count == 1 or role != 3 or leaders > 1, "Promote another member to Overlord before leaving", 409)
            self.directory.matchmaking.clan_membership_changing(who)
            if count == 1:
                self.db.execute("DELETE FROM clans WHERE id=?", (cid,))
            else:
                self.db.execute("DELETE FROM clan_members WHERE player=?", (who,))
        elif action == "broadcast":
            require(role >= 1, "Member role required to broadcast", 403)
            message = text(body["message"], 256)
            name = self.db.execute("SELECT screen_name FROM player_profiles WHERE player=?", (who,)).fetchone()[0]
            self.db.execute("INSERT INTO clan_messages VALUES(?,?,?,?,?,?)", (str(uuid.uuid4()), cid, who, name, message, now))
            self.db.execute("DELETE FROM clan_messages WHERE clan=? AND id NOT IN (SELECT id FROM clan_messages WHERE clan=? ORDER BY created DESC,rowid DESC LIMIT 100)", (cid, cid))

    def routes(self):
        return [web.post("/v1/account/session", self.login), web.delete("/v1/account/session", self.logout),
                web.get("/v1/clans", self.get), web.post("/v1/clans/commands", self.command)]
