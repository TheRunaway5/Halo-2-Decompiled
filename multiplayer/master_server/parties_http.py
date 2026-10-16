"""Accepted native parties shared by private account and queue identities.

Invitation expiry, consent, leader transfer and HTTP policy are explicitly
native service rules. Source UI/queue seams: online-party-integration-evidence.md.
"""
import re
import secrets
import time
import uuid

from aiohttp import web
from .protocol import canonical, fields, identifier, integer, require, text


class PartyApi:
    def __init__(self, directory, *, wall=time.time):
        self.directory, self.db, self.wall = directory, directory.store.db, wall
        self.db.executescript("""
        CREATE TABLE IF NOT EXISTS parties(id TEXT PRIMARY KEY,
          leader TEXT NOT NULL REFERENCES players(id), version INTEGER NOT NULL);
        CREATE TABLE IF NOT EXISTS party_members(player TEXT PRIMARY KEY REFERENCES players(id),
          party TEXT NOT NULL REFERENCES parties(id) ON DELETE CASCADE,
          handle TEXT NOT NULL UNIQUE, ready_playlist TEXT NOT NULL DEFAULT '',
          ready_revision TEXT NOT NULL DEFAULT '', ready_version INTEGER NOT NULL DEFAULT 0);
        CREATE INDEX IF NOT EXISTS party_roster ON party_members(party);
        CREATE TABLE IF NOT EXISTS party_invites(id TEXT PRIMARY KEY,
          party TEXT NOT NULL REFERENCES parties(id) ON DELETE CASCADE,
          target TEXT NOT NULL REFERENCES players(id), inviter TEXT NOT NULL REFERENCES players(id),
          expires INTEGER NOT NULL, UNIQUE(party,target));
        CREATE TABLE IF NOT EXISTS party_blocks(player TEXT NOT NULL REFERENCES players(id),
          inviter TEXT NOT NULL REFERENCES players(id), PRIMARY KEY(player,inviter));
        CREATE TABLE IF NOT EXISTS party_requests(player TEXT NOT NULL REFERENCES players(id),
          request TEXT NOT NULL, payload TEXT NOT NULL, expires INTEGER NOT NULL,
          PRIMARY KEY(player,request));
        CREATE TABLE IF NOT EXISTS party_activity_links(
          party TEXT PRIMARY KEY REFERENCES parties(id) ON DELETE CASCADE,
          version INTEGER NOT NULL, alias TEXT NOT NULL UNIQUE,
          secret TEXT NOT NULL UNIQUE, expires INTEGER NOT NULL);
        """)
        # Accepted membership persists across a restart; search consent does not.
        with self.db:
            self.db.execute("UPDATE party_members SET ready_playlist='',ready_revision='',ready_version=0")

    def cleanup(self):
        with self.db:
            now = int(self.wall())
            self.db.execute("DELETE FROM party_invites WHERE expires<=?", (now,))
            self.db.execute("DELETE FROM party_requests WHERE expires<=?", (now,))
            self.db.execute("DELETE FROM party_activity_links WHERE expires<=?", (now,))

    def party(self, who):
        return self.db.execute("SELECT p.id,p.leader,p.version FROM parties p JOIN party_members m "
                               "ON m.party=p.id WHERE m.player=?", (who,)).fetchone()

    def members(self, party):
        return self.db.execute("SELECT m.player,m.handle,p.screen_name,m.ready_playlist,m.ready_revision,m.ready_version "
                               "FROM party_members m JOIN player_profiles p ON p.player=m.player "
                               "WHERE m.party=? ORDER BY m.rowid", (party,)).fetchall()

    def queue_session(self, who):
        api = self.directory.matchmaking
        return next((s for s in api.sessions.values() if s.player == who and s.expires > api.clock()), None)

    def online(self, who):
        from .player_presence import observed
        return who in observed(self.directory)

    def playing(self, who):
        transport = getattr(self.directory.matchmaking, "transport", None)
        return transport is not None and any(r.active_member(who) and not r.closed and r.phase != "postgame"
                                            for r in transport.rooms.values())

    def changed(self, party, *, admitted=False):
        # Call only within the enclosing DB transaction. The event loop performs
        # no await between authenticated authorization and this queue mutation.
        api = self.directory.matchmaking
        if api.core:
            for ticket in tuple(api.core.tickets.values()):
                if ticket.party_id == party:
                    api.core.cancel(ticket.id)
        self.db.execute("UPDATE parties SET version=version+1 WHERE id=?", (party,))
        self.db.execute("UPDATE party_members SET ready_playlist='',ready_revision='',ready_version=0 WHERE party=?", (party,))
        if admitted and len(self.members(party)) < 16:
            # An accepted, separately targeted invitation adds a member to the
            # same Discord party. Keep its bounded lease and presentation alias
            # so other invitations already sent remain usable. No lease renewal.
            self.db.execute("UPDATE party_activity_links SET version=version+1 WHERE party=? "
                            "AND version=(SELECT version-1 FROM parties WHERE id=?)", (party,party))
            self.db.execute("DELETE FROM party_activity_links WHERE party=? "
                            "AND version!=(SELECT version FROM parties WHERE id=?)", (party,party))
        else:
            self.db.execute("DELETE FROM party_activity_links WHERE party=?", (party,))

    def session_lost(self, who):
        party = self.party(who)
        if party:
            with self.db: self.changed(party[0])

    def view(self, who, *, local_players=False):
        from .player_presence import observed
        presence = observed(self.directory)
        party = self.party(who)
        value = None
        if party:
            pid, leader, version = party
            value = {"version": version, "leader": leader == who,
                     "members": [{"handle": r[1], "name": r[2], "self": r[0] == who,
                                  "leader": r[0] == leader, "online": r[0] in presence,
                                  "ready_playlist": r[3] if r[5] == version else ""}
                                 for r in self.members(pid)],
                     "outgoing": [{"id": r[0], "name": r[1]} for r in self.db.execute(
                         "SELECT i.id,p.screen_name FROM party_invites i JOIN player_profiles p ON p.player=i.target "
                         "WHERE i.party=? ORDER BY i.expires,i.id", (pid,))] if leader == who else []}
        if value and local_players:
            for member, stored in zip(value["members"], self.members(party[0])):
                session = self.queue_session(stored[0])
                if session and len(session.seats()) > 1:
                    member["local_players"] = [{"name": p.screen_name, "guest": p.guest} for p in session.seats()[1:]]
        incoming = [{"id": r[0], "name": r[1], "version": r[2]} for r in self.db.execute(
            "SELECT i.id,p.screen_name,g.version FROM party_invites i JOIN parties g ON g.id=i.party "
            "JOIN player_profiles p ON p.player=g.leader WHERE i.target=? ORDER BY i.expires,i.id LIMIT 100", (who,))]
        return {"party": value, "invitations": incoming}

    async def get(self, request):
        self.cleanup()
        _, who = self.directory.clans.session(request)
        return web.json_response(self.view(who, local_players=request.query.get("local_players") == "1"))

    async def activity(self, request):
        """Private SDK handoff; possession does not replace a target invitation.

        Ordinary party views retain their legacy schema and never contain the
        secret. The alias is independent of private account/party identifiers.
        A conditional refresh never creates or renews a link. Accepted member
        additions retain it; departure, leadership/consent changes and full
        parties revoke it.
        """
        body = await self.directory.body(request)
        fields(body, ("version",), ("join_secret",))
        expected = integer(body["version"], 1, 2**63-1)
        previous = text(body["join_secret"],48) if "join_secret" in body else None
        if previous is not None:
            require(re.fullmatch(r"h2p1_[A-Za-z0-9_-]{43}",previous), "Invalid party join link")
        self.cleanup()
        _, who = self.directory.clans.session(request)
        with self.db:
            party = self.party(who)
            require(party is not None, "Create or join a party first", 409)
            pid, leader, version = party
            require(leader == who, "Only the party leader can do that", 403)
            require(version == expected, "Party changed; refresh it", 409)
            members = self.members(pid)
            require(len(members) < 16, "Party is full", 409)
            require(not any(self.playing(m[0]) for m in members), "Party is in a match", 409)
            now = int(self.wall())
            link = self.db.execute("SELECT version,alias,secret,expires FROM party_activity_links WHERE party=?", (pid,)).fetchone()
            if previous is not None:
                require(link and link[0] == version and link[2] == previous and link[3] > now,
                        "Party invitation unavailable", 404)
            if not link or link[0] != version or link[3] <= now:
                link = (version, str(uuid.uuid4()), "h2p1_" + secrets.token_urlsafe(32), now+600)
                self.db.execute("INSERT OR REPLACE INTO party_activity_links VALUES(?,?,?,?,?)", (pid, *link))
            return web.json_response({"party_id": link[1], "version": version,
                                      "current_size": len(members), "max_size": 16,
                                      "join_secret": link[2], "expires_in": link[3]-now})

    def accept_invitation(self, who, current_party, invitation_id, expected, *, activity=False):
        """Shared atomic membership gate for native and Discord acceptance."""
        invitation = self.db.execute("SELECT party,inviter FROM party_invites WHERE id=? AND target=? AND expires>?",
                                     (invitation_id, who, int(self.wall()))).fetchone()
        require(invitation is not None, "Party invitation not found", 404)
        pid, inviter = invitation
        require(current_party is None and not self.playing(who), "Leave the current party or match first", 409)
        current = self.db.execute("SELECT version,leader FROM parties WHERE id=?", (pid,)).fetchone()
        require(current and current[0] == expected, "Party changed; refresh invitations", 409)
        require(not self.db.execute("SELECT 1 FROM party_blocks WHERE player=? AND inviter=?", (who,inviter)).fetchone(),
                "Party invitation unavailable", 403)
        if activity:
            # Last successfully synchronized Discord blocks also apply to an
            # old activity link. They are preferences, not friendship proof.
            require(self.directory.voice_blocks.allowed(who,inviter) and
                    self.directory.voice_blocks.allowed(who,current[1]), "Party invitation unavailable", 403)
        members = self.members(pid)
        require(len(members) < 16, "Party is full", 409)
        require(not any(self.playing(m[0]) for m in members), "Party is in a match", 409)
        ticket = self.directory.matchmaking.ticket(who) if self.directory.matchmaking.core else None
        if ticket: self.directory.matchmaking.core.cancel(ticket.id)
        self.db.execute("INSERT INTO party_members(player,party,handle) VALUES(?,?,?)", (who,pid,str(uuid.uuid4())))
        self.db.execute("DELETE FROM party_invites WHERE target=?", (who,))
        self.changed(pid, admitted=True)

    def invite(self, party, who, target):
        require(target != who, "Cannot invite yourself")
        require(not self.db.execute("SELECT 1 FROM party_members WHERE party=? AND player=?", (party, target)).fetchone(),
                "Player is already in this party", 409)
        require(not self.db.execute("SELECT 1 FROM party_blocks WHERE player=? AND inviter=?", (target, who)).fetchone(),
                "Player does not accept these invitations", 403)
        require(self.db.execute("SELECT count(*) FROM party_invites WHERE party=?", (party,)).fetchone()[0] < 100 and
                self.db.execute("SELECT count(*) FROM party_invites WHERE target=?", (target,)).fetchone()[0] < 100,
                "Party invitation capacity reached", 429)
        self.db.execute("INSERT INTO party_invites VALUES(?,?,?,?,?) ON CONFLICT(party,target) DO NOTHING",
                        (str(uuid.uuid4()), party, target, who, int(self.wall())+600))

    async def command(self, request):
        body = await self.directory.body(request)
        fields(body, ("action", "request_id"), ("version", "username", "discord_id", "target", "playlist_id", "revision", "join_secret"))
        action, rid = text(body["action"]), identifier(body["request_id"])
        options = {"create": (), "invite": ("version","username"), "invite_discord": ("version","discord_id"), "invite_clan": ("version",),
                   "accept": ("version","target"), "decline": ("target",), "block": ("target",),
                   "cancel": ("version","target"), "leave": ("version",),
                   "remove": ("version","target"), "transfer": ("version","target"),
                   "ready": ("version","playlist_id","revision"), "unready": ("version",),
                   "join_activity": ("join_secret",)}
        require(action in options and set(body) == {"action","request_id",*options.get(action,())}, "Invalid party command")
        self.cleanup()
        _, who = self.directory.clans.session(request)
        payload = canonical(body).decode()
        db = self.db
        with db:
            old = db.execute("SELECT payload FROM party_requests WHERE player=? AND request=?", (who,rid)).fetchone()
            if old:
                require(old[0] == payload, "Party request ID already used differently", 409)
                return web.json_response(self.view(who))
            require(db.execute("SELECT count(*) FROM party_requests WHERE player=?", (who,)).fetchone()[0] < 256,
                    "Party command limit reached", 429)
            party = self.party(who)
            if action == "create":
                require(party is None and not self.playing(who), "Leave the current party or match first", 409)
                require(db.execute("SELECT count(*) FROM parties").fetchone()[0] < 1000, "Party capacity reached", 503)
                pid = str(uuid.uuid4())
                db.execute("INSERT INTO parties VALUES(?,?,1)",(pid,who))
                db.execute("INSERT INTO party_members(player,party,handle) VALUES(?,?,?)",(who,pid,str(uuid.uuid4())))
                ticket = self.directory.matchmaking.ticket(who) if self.directory.matchmaking.core else None
                if ticket: self.directory.matchmaking.core.cancel(ticket.id)
            elif action == "join_activity":
                secret = text(body["join_secret"], 48)
                require(re.fullmatch(r"h2p1_[A-Za-z0-9_-]{43}", secret), "Invalid party join link")
                link = db.execute("SELECT party,version FROM party_activity_links WHERE secret=? AND expires>?",
                                  (secret,int(self.wall()))).fetchone()
                require(link is not None, "Party invitation unavailable", 404)
                invitation = db.execute("SELECT id FROM party_invites WHERE party=? AND target=?", (link[0],who)).fetchone()
                require(invitation is not None, "Party invitation unavailable", 404)
                self.accept_invitation(who, party, invitation[0], link[1], activity=True)
            elif action in ("accept","decline","block"):
                invitation = db.execute("SELECT party,inviter FROM party_invites WHERE id=? AND target=?",
                                        (identifier(body["target"]),who)).fetchone()
                require(invitation is not None, "Party invitation not found", 404)
                pid, inviter = invitation
                if action == "accept":
                    self.accept_invitation(who, party, body["target"], integer(body["version"],1,2**63-1))
                elif action == "block":
                    db.execute("INSERT OR IGNORE INTO party_blocks VALUES(?,?)", (who,inviter))
                db.execute("DELETE FROM party_invites WHERE id=?", (body["target"],))
            else:
                require(party is not None, "Create or join a party first", 409)
                pid, leader, version = party
                require(version == integer(body["version"],1,2**63-1), "Party changed; refresh it", 409)
                require(not any(self.playing(m[0]) for m in self.members(pid)), "Leave the current match first", 409)
                if action not in ("leave","ready","unready"):
                    require(leader == who, "Only the party leader can do that", 403)
                if action == "invite":
                    username = text(body["username"],32).lower()
                    require(re.fullmatch(r"[a-z0-9_.]{2,32}",username), "Use the verified Discord username")
                    target = db.execute("SELECT player FROM player_profiles WHERE discord_username=?", (username,)).fetchone()
                    require(target is not None, "Player must sign into ONLINE first", 404)
                    self.invite(pid,who,target[0])
                elif action == "invite_discord":
                    # SDK selections identify a stable Discord subject. A
                    # recycled username must not invite its former owner.
                    # This is the existing known-account invitation policy,
                    # not a claim of server-side Discord friendship proof.
                    subject = text(body["discord_id"],20)
                    require(re.fullmatch(r"[1-9][0-9]{16,19}",subject), "Invalid Discord invitation target")
                    target = db.execute("SELECT id FROM players WHERE provider='discord' AND subject=? UNION ALL "
                                        "SELECT player FROM player_identities WHERE provider='discord' AND subject=? LIMIT 1", (subject,subject)).fetchone()
                    require(target is not None, "Player must sign into ONLINE first", 404)
                    self.invite(pid,who,target[0])
                elif action == "invite_clan":
                    clan = self.directory.clans.member(who)
                    require(clan is not None and clan[1] >= 1, "Clan recruiting permission required", 403)
                    existing = {r[0] for r in self.members(pid)}
                    targets = [r[0] for r in db.execute("SELECT player FROM clan_members WHERE clan=?", (clan[0],))
                               if r[0] not in existing and self.online(r[0]) and not db.execute(
                                   "SELECT 1 FROM party_blocks WHERE player=? AND inviter=?", (r[0],who)).fetchone()]
                    require(targets, "No eligible online clan members to invite", 409)
                    for target in targets: self.invite(pid,who,target)
                elif action == "cancel":
                    changed = db.execute("DELETE FROM party_invites WHERE id=? AND party=?", (identifier(body["target"]),pid)).rowcount
                    require(changed, "Party invitation not found", 404)
                elif action in ("leave","remove","transfer"):
                    target = who
                    if action != "leave":
                        member = db.execute("SELECT player FROM party_members WHERE party=? AND handle=?", (pid,identifier(body["target"]))).fetchone()
                        require(member is not None, "Party member not found", 404)
                        target = member[0]
                        require(target != who, "Use Leave Party for yourself")
                    if action == "transfer":
                        db.execute("UPDATE parties SET leader=? WHERE id=?", (target,pid))
                    else:
                        db.execute("DELETE FROM party_members WHERE player=?", (target,))
                        remaining = self.members(pid)
                        if not remaining: db.execute("DELETE FROM parties WHERE id=?", (pid,))
                        elif target == leader: db.execute("UPDATE parties SET leader=? WHERE id=?", (remaining[0][0],pid))
                    self.changed(pid)
                elif action == "ready":
                    api = self.directory.matchmaking
                    api.enabled()
                    policy = api.core.playlists.get(text(body["playlist_id"]))
                    require(policy is not None and policy.revision == text(body["revision"]), "Playlist changed; refresh it", 409)
                    session = self.queue_session(who)
                    require(session is not None and api.eligibility(session,policy), "Open matchmaking with the required maps first", 409)
                    require(len(self.members(pid)) <= policy.maximum_party, "Party is too large for this playlist", 409)
                    ticket = api.ticket(who)
                    if ticket is not None and ticket.playlist != policy.id:
                        self.changed(pid)
                        version += 1
                    db.execute("UPDATE party_members SET ready_playlist=?,ready_revision=?,ready_version=? WHERE player=?",
                               (policy.id,policy.revision,version,who))
                elif action == "unready":
                    self.changed(pid)
            db.execute("INSERT INTO party_requests VALUES(?,?,?,?)", (who,rid,payload,int(self.wall())+86400))
            return web.json_response(self.view(who))

    def queue_members(self, who, policy):
        """Authoritative accepted roster and consent, frozen without an await."""
        party = self.party(who)
        if party is None:
            session = self.queue_session(who)
            require(session is not None, "Matchmaking session expired; reconnect", 401)
            return who, (session,)
        pid, leader, version = party
        require(leader == who, "Only the party leader can start a search", 403)
        members = self.members(pid)
        require(len(members) <= policy.maximum_party, "Party is too large for this playlist", 409)
        require(all(r[0] == who or (r[3],r[4],r[5]) == (policy.id,policy.revision,version) for r in members),
                "Every party member must agree to this playlist", 409)
        sessions = tuple(self.queue_session(r[0]) for r in members)
        require(all(s is not None for s in sessions), "Every party member must open matchmaking", 409)
        require(len({s.compatibility for s in sessions}) == 1, "Party members need compatible game builds", 409)
        require(not any(self.playing(r[0]) for r in members), "A party member is still in a match", 409)
        return pid, sessions

    def routes(self):
        return [web.get("/v1/party",self.get),web.post("/v1/party/commands",self.command),
                web.post("/v1/party/activity",self.activity)]
