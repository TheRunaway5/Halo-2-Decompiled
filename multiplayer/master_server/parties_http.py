"""Accepted native parties shared by private account and queue identities.

Invitation expiry, consent, leader transfer and HTTP policy are explicitly
native service rules. Source UI/queue seams: online-party-integration-evidence.md.
"""
import re
import asyncio
import contextlib
import secrets
import time
import uuid

from aiohttp import web
from .protocol import canonical, fields, identifier, integer, require, text
from .native_party_countdowns import Countdowns
from . import party_privacy
from .native_matchmaking import value as native_value
from .native_database import apply as apply_plan


class PartyApi:
    def __init__(self, directory, *, wall=time.time, clock=time.monotonic):
        self.directory, self.db, self.wall = directory, directory.store.db, wall
        self.clock=clock
        self._countdowns=Countdowns(self)
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
        CREATE TABLE IF NOT EXISTS party_privacy(
          party TEXT PRIMARY KEY REFERENCES parties(id) ON DELETE CASCADE,
          mode INTEGER NOT NULL CHECK(mode IN (0,1,2)),
          revision INTEGER NOT NULL CHECK(revision>0));
        """)
        # Accepted membership persists; native search consent is connection-bound.
        with self.db:
            columns = [row[1] for row in self.db.execute('PRAGMA table_info(matchmaking_party_consents)')]
            apply_plan(self.db, native_value('party_consent_schema', columns=columns))
            self.db.execute("INSERT OR IGNORE INTO party_privacy SELECT id,1,1 FROM parties")

    def matching_facts(self, who=None, *, party_id=None):
        """Read accepted directory facts; Rust decides readiness and admission."""
        if party_id is None:
            party = self.party(who)
        else:
            party = self.db.execute('SELECT id,leader,version FROM parties WHERE id=?', (party_id,)).fetchone()
        members = self.members(party[0]) if party else ()
        records = []
        for row in members:
            consent = self.db.execute('SELECT party,version,handle,playlist,revision,profile,session_key,generation,roster '
                                      'FROM matchmaking_party_consents WHERE player=?', (row[0],)).fetchone()
            records.append(dict(player=row[0], handle=row[1], ready_playlist=row[3], ready_revision=row[4], ready_version=row[5],
                consent=dict(zip(('party','version','handle','playlist','revision','profile','session_key','generation','roster'), consent)) if consent else None))
        api = self.directory.matchmaking
        owners = [row[0] for row in members] if party else ([who] if who is not None else [])
        seats = api.core._call('session_directory_seats', owners=owners) if api.core else ()
        clans = [dict(player=player, clan=api.member_clan(player)) for player in seats]
        return dict(party=dict(id=party[0], leader=party[1], version=party[2], members=records) if party else None, clans=clans)

    def matching_effects(self, plan):
        api = self.directory.matchmaking
        if api.core:
            api.core._call('party_effects', effects=plan['effects'])

    @property
    def countdowns(self):
        return self._countdowns

    def privacy_state(self, party):
        row=self.db.execute("SELECT mode,revision FROM party_privacy WHERE party=?",(party,)).fetchone()
        # Older tools can still create the original three-column party row.
        # Such a party retains its real invitation-only admission until changed.
        return row if row is not None else (1,1)

    def seat_count(self, who):
        session=self.queue_session(who)
        return len(session.seats()) if session else 1

    def searching(self, who):
        api=self.directory.matchmaking
        if not api.core:return False
        ticket=api.ticket(who)
        if ticket is None:return False
        now=api.clock()
        return ticket.expires_at>now or any(p.expires_at>now and ticket.id in p.tickets
                                          for p in api.core.proposals.values())

    def admission(self, party, who, *, invited):
        mode,_=self.privacy_state(party)
        members=self.members(party)
        available=max(0,16-sum(self.seat_count(m[0]) for m in members))
        status=party_privacy.admission(mode,invited,self.seat_count(who),available)
        require(status!=8,"Party is closed" if mode==2 else "A party invitation is required",403)
        require(status!=7 and len(members)<16,"Party is full",409)

    def friend_joinable(self, who, target):
        if who==target or not self.directory.friends.accepted(who,target) or not self.online(target):
            return False
        party=self.party(target)
        if not party or self.party(who) or self.playing(who) or self.searching(who):return False
        pid,leader,_=party
        if self.directory.friends.blocked(who,leader) or self.db.execute(
            "SELECT 1 FROM party_blocks WHERE player=? AND inviter IN (?,?)",(who,target,leader)).fetchone():
            return False
        members=self.members(pid)
        if any(self.playing(m[0]) or self.searching(m[0]) for m in members):return False
        mode,_=self.privacy_state(pid)
        available=max(0,16-sum(self.seat_count(m[0]) for m in members))
        return len(members)<16 and party_privacy.admission(mode,False,self.seat_count(who),available)==0

    def friend_context(self, who, target):
        if who==target or not self.directory.friends.accepted(who,target) or not self.online(target):return None
        member=self.db.execute("SELECT handle FROM party_members WHERE player=?",(target,)).fetchone()
        return member[0] if member else None

    def admit_member(self, who, pid):
        ticket=self.directory.matchmaking.ticket(who) if self.directory.matchmaking.core else None
        if ticket:self.directory.matchmaking.core.cancel(ticket.id)
        self.db.execute("INSERT INTO party_members(player,party,handle) VALUES(?,?,?)",(who,pid,str(uuid.uuid4())))
        self.db.execute("DELETE FROM party_invites WHERE target=?",(who,))
        self.changed(pid,admitted=True)

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
        # The caller owns the enclosing SQLite transaction. These invalidation
        # effects grant no authority and preserve the existing conservative
        # cancellation behavior if a later enclosing statement rolls back.
        plan = native_value('party_change_plan', party=party,
                            facts=self.matching_facts(party_id=party), admitted=admitted)
        apply_plan(self.db, plan)
        self.matching_effects(plan)

    def session_lost(self, who):
        party = self.party(who)
        if party:
            with self.db: self.changed(party[0])

    def postgame_players(self):
        """Players still connected to a completed match's carnage report.

        TU1.5 party members stay on the report until their leader's Done
        returns party A to the pregame lobby; this is that leader signal.
        """
        transport = getattr(self.directory, "match_transport", None)
        return {who for room in (transport.rooms.values() if transport else ())
                if not room.closed and room.phase in ("reviewing", "postgame")
                for who, peer in room.peers.items() if not peer.socket.closed}

    def advance_countdowns(self):
        core = self.directory.matchmaking.core
        if core and self._countdowns:
            versions = [dict(id=row[0], version=row[1]) for row in self.db.execute('SELECT id,version FROM parties LIMIT 1001')]
            self._countdowns.refresh(core._call('party_countdown_tick', clock_ms=int(self.clock()*1000), versions=versions))

    async def lifetime(self, app):
        async def pump():
            while True:
                self.advance_countdowns()
                await asyncio.sleep(.05)
        task=asyncio.create_task(pump())
        try:
            yield
        finally:
            task.cancel()
            with contextlib.suppress(asyncio.CancelledError):
                await task

    def countdown_view(self, who):
        core = self.directory.matchmaking.core
        if core:
            return core._call('party_countdown_view', player=who, facts=self.matching_facts(who))
        party = self.party(who)
        return dict(version=party[2] if party else None, countdown=None)

    def countdown_command(self, who, body):
        """Execute native SQL guards before committing native publication."""
        api = self.directory.matchmaking
        api.enabled()
        token = None
        try:
            with self.db:
                previous = self.db.execute('SELECT payload FROM party_requests WHERE player=? AND request=?',
                                           (who, body['request_id'])).fetchone()
                count = self.db.execute('SELECT count(*) FROM party_requests WHERE player=?', (who,)).fetchone()[0]
                plan = api.core._call('party_countdown_prepare', player=who, body=body,
                    facts=self.matching_facts(who), rooms=api.room_facts(), clock_ms=int(self.clock()*1000),
                    wall_seconds=int(self.wall()), requests=dict(payload=previous[0] if previous else None, count=count))
                token = plan['token']
                apply_plan(self.db, plan)
            if token is not None:
                rows = api.core._call('party_countdown_commit', token=token,
                    facts=self.matching_facts(who), rooms=api.room_facts())
                self._countdowns.refresh(rows)
        finally:
            if token is not None:
                api.core._call('party_countdown_abort', token=token)

    async def get_countdown(self, request):
        # Same authenticated account capability, independent bounded bucket.
        # Fast countdown reads do not consume the social-list polling budget.
        _,who=self.directory.clans.session(request, rate=False)
        self.directory.rate("party-countdown:"+who,600)
        return web.json_response(self.countdown_view(who))

    def view(self, who, *, local_players=False, appearance=False, postgame=False, countdown=False, privacy=False):
        from .player_presence import observed
        presence = observed(self.directory)
        reviewing = self.postgame_players() if postgame else set()
        party = self.party(who)
        value = None
        if party:
            pid, leader, version = party
            members = self.members(pid)
            api = self.directory.matchmaking
            ready = api.core._call('party_ready_view', facts=self.matching_facts(who)) if api.core else [''] * len(members)
            value = {"version": version, "leader": leader == who,
                     "members": [{"handle": r[1], "name": r[2], "self": r[0] == who,
                                  "leader": r[0] == leader, "online": r[0] in presence,
                                  "ready_playlist": ready[index],
                                  **({"postgame": r[0] in reviewing} if postgame else {})}
                                 for index, r in enumerate(members)],
                     "outgoing": [{"id": r[0], "name": r[1]} for r in self.db.execute(
                         "SELECT i.id,p.screen_name FROM party_invites i JOIN player_profiles p ON p.player=i.target "
                         "WHERE i.party=? ORDER BY i.expires,i.id", (pid,))] if leader == who else []}
            if countdown:
                value["countdown"]=self.countdown_view(who)["countdown"]
            if privacy:
                mode,revision=self.privacy_state(pid)
                context=next(m["handle"] for m in value["members"] if m["self"])
                value["privacy"]=dict(mode=party_privacy.mode(mode),revision=revision,context=context)
        def profile(player):
            session=self.queue_session(player)
            if session:
                seat=next((p for p in session.seats() if p.player==player),None)
                if seat and seat.appearance is not None:return seat.appearance.hex()
            row = self.db.execute("SELECT appearance FROM player_customization WHERE player=?", (player,)).fetchone()
            return bytes(row[0]).hex() if row else None

        if value and (local_players or appearance):
            for member, stored in zip(value["members"], self.members(party[0])):
                if appearance:
                    member["appearance"] = profile(stored[0])
                session = self.queue_session(stored[0])
                if local_players and session and len(session.seats()) > 1:
                    member["local_players"] = [dict(name=p.screen_name, guest=p.guest,
                        **({"appearance": p.appearance.hex() if p.appearance is not None else profile(p.player) if not p.guest else None} if appearance else {}))
                        for p in session.seats()[1:]]
        incoming = [{"id": r[0], "name": r[1], "version": r[2]} for r in self.db.execute(
            "SELECT i.id,p.screen_name,g.version FROM party_invites i JOIN parties g ON g.id=i.party "
            "JOIN player_profiles p ON p.player=g.leader WHERE i.target=? ORDER BY i.expires,i.id LIMIT 100", (who,))]
        return {"party": value, "invitations": incoming}

    async def get(self, request):
        self.cleanup()
        _, who = self.directory.clans.session(request)
        return web.json_response(self.view(who, local_players=request.query.get("local_players") == "1",
                                           appearance=request.query.get("appearance") == "1",
                                           postgame=request.query.get("postgame") == "1",
                                           countdown=request.query.get("countdown") == "1",
                                           privacy=request.query.get("privacy") == "1"))

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
        require(not self.directory.friends.blocked(who,inviter), "Party invitation unavailable",403)
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
        self.admission(pid,who,invited=True)
        require(not any(self.playing(m[0]) for m in members), "Party is in a match", 409)
        self.admit_member(who,pid)

    def invite(self, party, who, target):
        require(target != who, "Cannot invite yourself")
        require(not self.directory.friends.blocked(who,target), "Party invitation unavailable",403)
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
        fields(body, ("action", "request_id"), ("version", "username", "discord_id", "steam_id", "target", "playlist_id", "revision", "join_secret", "privacy", "privacy_revision", "privacy_context", "target_context"))
        action, rid = text(body["action"]), identifier(body["request_id"])
        options = {"create": (), "invite": ("version","username"), "invite_friend": ("version","target"), "invite_discord": ("version","discord_id"), "invite_steam": ("version","steam_id"), "invite_member": ("version","target"), "invite_clan": ("version",),
                   "accept": ("version","target"), "decline": ("target",), "block": ("target",),
                   "cancel": ("version","target"), "leave": ("version",), "leave_lobby": ("version",),
                   "remove": ("version","target"), "transfer": ("version","target"),
                   "ready": ("version","playlist_id","revision"), "unready": ("version",),
                   "countdown_start": ("version","playlist_id","revision"),
                   "countdown_cancel": ("version",), "countdown_request": ("version",),
                   "privacy": ("version","privacy","privacy_revision","privacy_context"), "join_friend": ("target","target_context"),
                   "join_activity": ("join_secret",)}
        require(action in options and set(body) == {"action","request_id",*options.get(action,())}, "Invalid party command")
        self.cleanup()
        _, who = self.directory.clans.session(request)
        if action in ('countdown_start', 'countdown_cancel', 'countdown_request'):
            self.countdown_command(who, body)
            return web.json_response(self.view(who,countdown=request.query.get("countdown")=="1",privacy=request.query.get("privacy")=="1"))
        payload = canonical(body).decode()
        previous=self.db.execute("SELECT payload FROM party_requests WHERE player=? AND request=?",(who,rid)).fetchone()
        if previous:
            require(previous[0]==payload,"Party request ID already used differently",409)
            return web.json_response(self.view(who,countdown=request.query.get("countdown")=="1",privacy=request.query.get("privacy")=="1"))
        if action=="invite_steam":
            subject=text(body["steam_id"],17)
            require(re.fullmatch(r"[1-9][0-9]{16}",subject),"Invalid Steam invitation target")
            friends=await self.directory.steam_friends.lookup(who)
            require(friends["state"]=="ready" and any(row["id"]==subject for row in friends["friends"]),
                    "Refresh Steam friends before inviting this player",403)
            # The upstream await cannot retain an expired account capability.
            _,who=self.directory.clans.session(request)
        db = self.db
        with db:
            old = db.execute("SELECT payload FROM party_requests WHERE player=? AND request=?", (who,rid)).fetchone()
            if old:
                require(old[0] == payload, "Party request ID already used differently", 409)
                return web.json_response(self.view(who,countdown=request.query.get("countdown")=="1",privacy=request.query.get("privacy")=="1"))
            require(db.execute("SELECT count(*) FROM party_requests WHERE player=?", (who,)).fetchone()[0] < 256,
                    "Party command limit reached", 429)
            party = self.party(who)
            if action == "create":
                require(party is None and not self.playing(who), "Leave the current party or match first", 409)
                require(db.execute("SELECT count(*) FROM parties").fetchone()[0] < 1000, "Party capacity reached", 503)
                party_privacy.mode(0) # The actual native admission leaf must exist before publishing OPEN.
                pid = str(uuid.uuid4())
                db.execute("INSERT INTO parties VALUES(?,?,1)",(pid,who))
                #59BD0 initializes a newly hosted original party to mode0.
                #This service also implements actual authenticated Open joins.
                db.execute("INSERT INTO party_privacy VALUES(?,0,1)",(pid,))
                db.execute("INSERT INTO party_members(player,party,handle) VALUES(?,?,?)",(who,pid,str(uuid.uuid4())))
                ticket = self.directory.matchmaking.ticket(who) if self.directory.matchmaking.core else None
                if ticket: self.directory.matchmaking.core.cancel(ticket.id)
            elif action == "join_friend":
                target=self.directory.friends.target(body["target"])
                require(self.directory.friends.accepted(who,target),"Friendship no longer available",403)
                require(party is None and not self.playing(who),"Leave the current party or match first",409)
                #19A250 suppresses local joining in manager states5..8;
                #72EE0 also rejects a target advertising states6/7/8. A
                #new social join must not cancel an actual queued/proposed
                #match and silently clear that party's existing consent.
                require(not self.searching(who),"Cancel matchmaking before joining a party",409)
                require(self.online(target),"Friend is offline",409)
                target_party=self.party(target)
                require(target_party is not None,"Friend has no party",404)
                pid,leader,_=target_party
                context=db.execute("SELECT handle FROM party_members WHERE player=? AND party=?",(target,pid)).fetchone()
                require(context and context[0]==identifier(body["target_context"]),"Friend's party changed; refresh it",409)
                require(not self.directory.friends.blocked(who,leader) and not db.execute(
                    "SELECT 1 FROM party_blocks WHERE player=? AND inviter IN (?,?)",(who,target,leader)).fetchone(),
                    "Party join unavailable",403)
                self.admission(pid,who,invited=False)
                require(not any(self.playing(m[0]) for m in self.members(pid)),"Party is in a match",409)
                require(not any(self.searching(m[0]) for m in self.members(pid)),"Party is matchmaking",409)
                self.admit_member(who,pid)
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
                if action not in ("leave","leave_lobby","ready","unready","countdown_request"):
                    require(leader == who, "Only the party leader can do that", 403)
                if action == "privacy":
                    require(party_privacy.can_change(True,leader==who),"Only the party leader can do that",403)
                    current_context=db.execute("SELECT handle FROM party_members WHERE party=? AND player=?",(pid,who)).fetchone()
                    require(current_context and current_context[0]==identifier(body["privacy_context"]),"Party changed; refresh it",409)
                    mode=party_privacy.mode(body["privacy"])
                    _,revision=self.privacy_state(pid)
                    require(revision==integer(body["privacy_revision"],1,2**63-1),"Party privacy changed; refresh it",409)
                    require(revision<2**63-1,"Party privacy revision exhausted",503)
                    #5BDE0 changes the parameter/update counter even when
                    #selecting its current value. Do not call changed():
                    #roster version, consent, active ticket and the actual
                    #published countdown stay intact. Replay IDs above keep
                    #the independently versioned command idempotent.
                    db.execute("INSERT INTO party_privacy VALUES(?,?,?) ON CONFLICT(party) DO UPDATE SET mode=excluded.mode,revision=excluded.revision",
                               (pid,mode,revision+1))
                elif action == "invite":
                    username = text(body["username"],73)
                    if "#" in username:
                        from .friend_tags import resolve
                        target = resolve(db,username)
                    else:
                        # Compatibility for older clients; current menus use tags.
                        require(re.fullmatch(r"[a-z0-9_.]{2,32}",username.lower()), "Enter a friend tag")
                        row = db.execute("SELECT player FROM player_profiles WHERE discord_username=?", (username.lower(),)).fetchone()
                        require(row is not None, "Player must sign into ONLINE first", 404)
                        target = row[0]
                    self.invite(pid,who,target)
                elif action == "invite_friend":
                    target = self.directory.friends.target(body["target"])
                    require(self.directory.friends.accepted(who,target), "Friendship no longer available",403)
                    self.invite(pid,who,target)
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
                elif action == "invite_steam":
                    target=db.execute("SELECT player FROM player_identities WHERE provider='steam' AND subject=?",(subject,)).fetchone()
                    require(target is not None,"Player must sign into ONLINE first",404)
                    require(not db.execute("SELECT 1 FROM party_blocks WHERE player=? AND inviter=?",(who,target[0])).fetchone(),"Player is blocked",403)
                    self.invite(pid,who,target[0])
                elif action == "invite_member":
                    target=identifier(body["target"])
                    clan=self.directory.clans.member(who)
                    require(clan is not None and db.execute("SELECT 1 FROM clan_members WHERE clan=? AND player=?",(clan[0],target)).fetchone(),
                            "Player is no longer in your clan",403)
                    require(self.online(target),"Clan member is offline",409)
                    self.invite(pid,who,target)
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
                elif action in ("leave","leave_lobby","remove","transfer"):
                    target = who
                    if action not in ("leave","leave_lobby"):
                        member = db.execute("SELECT player FROM party_members WHERE party=? AND handle=?", (pid,identifier(body["target"]))).fetchone()
                        require(member is not None, "Party member not found", 404)
                        target = member[0]
                        require(target != who, "Use Leave Party for yourself")
                    if action == "transfer":
                        db.execute("UPDATE parties SET leader=? WHERE id=?", (target,pid))
                    elif action == "leave_lobby" and who == leader and len(self.members(pid)) == 2:
                        # Original251345 confirms a two-member host's0x8B:
                        # kick the sole remote19A179(NONE), then leave both
                        # sessions199E3C. Neither member retains that party.
                        db.execute("DELETE FROM parties WHERE id=?", (pid,))
                    else:
                        db.execute("DELETE FROM party_members WHERE player=?", (target,))
                        remaining = self.members(pid)
                        if not remaining: db.execute("DELETE FROM parties WHERE id=?", (pid,))
                        elif target == leader: db.execute("UPDATE parties SET leader=? WHERE id=?", (remaining[0][0],pid))
                    self.changed(pid)
                elif action == "ready":
                    api = self.directory.matchmaking
                    api.enabled()
                    plan = api.core._call('party_ready', player=who, playlist=body['playlist_id'],
                        revision=body['revision'], version=body['version'], facts=self.matching_facts(who),
                        rooms=api.room_facts())
                    apply_plan(db, plan)
                    self.matching_effects(plan)
                elif action == "unready":
                    self.changed(pid)
            db.execute("INSERT INTO party_requests VALUES(?,?,?,?)", (who,rid,payload,int(self.wall())+86400))
            return web.json_response(self.view(who,countdown=request.query.get("countdown")=="1",privacy=request.query.get("privacy")=="1"))

    def queue_members(self, who, policy):
        api = self.directory.matchmaking
        api.enabled()
        selected = api.core._call('party_select', player=who, playlist=policy.id,
                                 facts=self.matching_facts(who), rooms=api.room_facts())
        return selected['party_id'], tuple(api.sessions.project(row) for row in selected['members'])

    def queue_prepare(self, session, body):
        api = self.directory.matchmaking
        facts = self.matching_facts(session.player)
        selected = api.core._call('party_queue_prepare', generation=session.generation, body=body,
                                 facts=facts, rooms=api.room_facts())
        return selected

    def routes(self):
        return [web.get("/v1/party",self.get),web.get("/v1/party/countdown",self.get_countdown),web.post("/v1/party/commands",self.command),
                web.post("/v1/party/activity",self.activity)]
