"""Discord-authorized native queue API. No client-supplied identities or ranks."""
from dataclasses import dataclass
from pathlib import Path
import time

from aiohttp import web

from .matchmaking import Coordinator, Playlist
from .native_matchmaking import project, value
from .native_database import run as database
from .protocol import require, Problem
from .player_identity import initialize, player as resolve_player, VERIFIED_DISCORD_USERNAME
from .playlist_skill import PlaylistSkills
from .clan_skill import ClanSkills
from .couch_players import verify_local_players
from .native_sessions import Sessions, Requests, Failures


@dataclass(frozen=True)
class CatalogEntry:
    policy: Playlist
    name: str
    description: str
    quickmatch: bool
    rank_configuration: str | None = None
    required_compatibility: str | None = None


def load_catalog(path):
    """Read local bytes; Rust parses and validates every catalog policy field."""
    if not path:
        return ()
    try:
        raw = Path(path).read_bytes().decode('utf-8')
    except UnicodeError:
        raise Problem(400, 'Invalid catalog UTF8 text') from None
    return tuple(project('catalog', row) for row in value('catalog_parse', raw=raw))


class MatchmakingApi:
    def __init__(self, directory, entries=(), *, clock=time.monotonic):
        self.directory, self.clock = directory, clock
        self.core = Coordinator(tuple(e.policy for e in entries), clock=lambda: self.clock(), catalog=entries) if entries else None
        self._sessions, self._requests, self._failures = Sessions(self), Requests(self), Failures(self)
        initialize(directory.store.db)
        directory.store.db.executescript("""
        CREATE TABLE IF NOT EXISTS players(id TEXT PRIMARY KEY, provider TEXT NOT NULL,
          subject TEXT NOT NULL, UNIQUE(provider,subject));
        CREATE TABLE IF NOT EXISTS playlist_ranks(player TEXT NOT NULL REFERENCES players(id),
          playlist TEXT NOT NULL, rank INTEGER NOT NULL CHECK(rank BETWEEN 1 AND 50),
          PRIMARY KEY(player,playlist));
        """)
        from .match_results import MatchResults
        self.skills = PlaylistSkills(directory.store.db)
        self.clan_skills = ClanSkills(directory.store.db)
        self.results = MatchResults(directory.store, self.skills, self.clan_skills)

    @property
    def sessions(self): return self._sessions
    @property
    def requests(self): return self._requests
    @property
    def failures(self): return self._failures

    @property
    def entries(self):
        return self.core.catalog if self.core else ()

    @entries.setter
    def entries(self, entries):
        self.enabled()
        self.core.catalog = tuple(entries)

    def enabled(self):
        require(self.core is not None, "Matchmaking playlists are not configured", 503)

    def ticket(self, player):
        return project('tickets', self.core._call('api_ticket', player=player)) if self.core else None

    def connection_owner(self, player):
        return self.core._call('api_connection_owner', player=player)

    def cleanup(self):
        if not self.core:
            return
        subjects = self.core._call('api_cleanup_subjects')
        self.core._call('api_cleanup', clans={who: self.member_clan(who) for who in subjects})
        self.core._call('session_cleanup')
        self.session_events()

    def session_events(self):
        """Deliver native retirement effects to the temporary socket/party IO."""
        if not self.core:
            return
        events = self.core._call('session_events')
        for event in events:
            qos = getattr(self, 'search_qos', None)
            if qos:
                qos.discard(event['player'],generation=event['generation'])
            parties = getattr(self.directory, 'parties', None)
            if parties:
                parties.session_lost(event['player'])
        gatherings = getattr(self, 'gatherings', None)
        if events and gatherings:
            gatherings.cleanup()

    def remove_session(self, key):
        self.core._call('session_revoke', key=key)
        self.session_events()

    def session(self, request):
        self.enabled()
        self.cleanup()
        row = self.core._call('session_auth', header=request.headers.get('Authorization', ''))
        return row['key'], self.sessions.project(row)

    async def create_session(self, request):
        self.enabled()
        body = await self.directory.body(request)
        plan = value('session_body', body=body)
        owner = tuple(value('session_verified_identity', owner=await self.directory.owner(request)))
        locals_verified = await verify_local_players(self.directory, plan)
        # All provider proofs complete before persistent identity IO. No await
        # separates the Rust preflight, canonical identity IO and admission.
        self.cleanup()
        db = self.directory.store.db
        owners = [owner] + [local_owner for _, local_owner, _ in locals_verified]
        identities = []
        parties = getattr(self.directory, 'parties', None)
        for account in owners:
            if account is None:
                identities.append(None)
                continue
            row = db.execute('SELECT player FROM player_identities WHERE provider=? AND subject=?', account).fetchone()
            player = row[0] if row else None
            identities.append(dict(owner=account, player=player,
                separate_party=bool(player and parties and parties.party(player))))
        transport = getattr(self, 'transport', None)
        rooms = [dict(closed=room.closed, phase=room.phase, owners=room.owners, active=tuple(room.tokens))
                 for room in transport.rooms.values()] if transport else []
        operands = dict(body=body, identities=identities, rooms=rooms)
        self.core._call('session_check', **operands)
        resolved = [resolve_player(db, owner, plan['screen_name'], request.get(VERIFIED_DISCORD_USERNAME, ''))]
        resolved.extend(resolve_player(db, account, name, username) if account else None
                        for name, account, username in locals_verified)
        admitted = self.core._call('session_register', resolved=resolved, **operands)
        self.sessions.project(admitted['session'])
        self.session_events()
        return web.json_response(admitted['response'], status=201)

    async def delete_session(self, request):
        key, _ = self.session(request)
        self.remove_session(key)
        return web.json_response({"state": "signed_out"})

    def rank(self, player, playlist):
        return database(self.directory.store.db, 'public_rank', source=self.core._handle,
                        playlist=playlist, player=player, clan=self.member_clan(player))

    def member_clan(self, player):
        member = self.directory.clans.member(player)
        return member[0] if member else None

    def clan_rank(self, clan, entry):
        # Search retains the full private raw range, including ranks sharing
        # the final visible artwork. Display level never replaces clan skill.
        return self.clan_skills.snapshot(clan, entry.policy.id, entry.rank_configuration).raw

    def common_clan(self, players, policy):
        return self.core.common_clan(policy.id, [dict(clan=self.member_clan(player)) for player in players])

    def clan_membership_changing(self, player):
        """Native authority policy: preserve admitted owners, revoke queued consent."""
        transport = getattr(self, "transport", None)
        require(transport is None or not any(r.active_member(player) and r.clans and not r.closed
                and r.phase != "postgame" for r in transport.rooms.values()), "Finish your clan match first", 409)
        ticket = self.ticket(player) if self.core else None
        if ticket and ticket.clan:
            self.directory.parties.changed(ticket.party_id)

    def eligibility_facts(self, session):
        # Only authenticated directory/party/seat facts cross this boundary;
        # Rust applies compatibility, maps, guest, size and clan policy.
        parties = getattr(self.directory, "parties", None)
        party = parties.party(session.player) if parties else None
        connections = [next((s for s in self.sessions.values() if s.player == r[0]), None)
                       for r in parties.members(party[0])] if party else [session]
        facts = [None if s is None else dict(id=s.player, compatibility=s.compatibility,
            maps=tuple(s.maps), seats=[dict(id=p.player, guest=p.guest, clan=self.member_clan(p.player))
                                       for p in s.seats()]) for s in connections]
        return facts

    def eligibility(self, session, policy):
        return self.core.eligible(policy.id, session.compatibility, self.eligibility_facts(session))

    async def playlists(self, request):
        _, session = self.session(request)
        facts = self.eligibility_facts(session)
        levels = {playlist: self.rank(session.player, playlist)
                  for playlist in self.core._call('catalog_display_requests')}
        return web.json_response(self.core._call('catalog_public_view',
            compatibility=session.compatibility, connections=facts, levels=levels))

    def room_facts(self):
        transport = getattr(self, 'transport', None)
        return [dict(closed=room.closed, phase=room.phase, owners=room.owners, active=tuple(room.tokens),
                     playlist=room.proposal.playlist, clans=tuple(room.clans.values()))
                for room in transport.rooms.values()] if transport else []

    def queue_room_facts(self):
        transport = getattr(self, 'transport', None)
        return [dict(id=room.id, closed=room.closed, phase=room.phase,
                     owners=room.owners, active=tuple(room.tokens))
                for room in transport.rooms.values()] if transport else []

    @staticmethod
    def query_facts(request):
        return {key: request.query.get(key) for key in request.query}

    def queue_io(self, event, session):
        """Execute only the bounded IO chosen by the native queue endpoint."""
        frame = None
        try:
            while event['kind'] == 'io':
                frame = event['frame']
                if event['action'] == 'database':
                    result = database(self.directory.store.db, 'snapshot_bank', operations=event['operations'])
                elif event['action'] == 'gathering_advance':
                    self.gatherings.advance(event['playlist'])
                    result = None
                else:
                    require(event['action'] == 'room_start', 'Invalid Rust queue IO action', 503)
                    room = self.transport.rooms.get(event['room'])
                    result = room.start_countdown_ms() if room else None
                event = self.core._call('api_resume', frame=frame, generation=session.generation,
                    result=result, facts=self.directory.parties.matching_facts(session.player),
                    match_rooms=self.room_facts(), rooms=self.queue_room_facts())
            require(event['kind'] == 'done', 'Invalid Rust queue endpoint response', 503)
            return event['value']
        finally:
            if frame is not None:
                self.core._call('api_abort', frame=frame)

    async def enqueue(self, request):
        body = value('session_queue_body', body=await self.directory.body(request))
        _, session = self.session(request)
        self.queue_io(self.core._call('api_enqueue_begin', generation=session.generation, body=body,
            facts=self.directory.parties.matching_facts(session.player), match_rooms=self.room_facts()), session)
        return web.json_response(self.view(session), status=201)

    def advance(self, playlist):
        self.cleanup()
        event = self.core._call('api_advance', playlist=playlist)
        if event['gathering'] is not None:
            self.gatherings.advance(event['gathering'])

    def player_appearance(self, player):
        plan = self.core._call('api_player_appearance', player=player)
        return database(self.directory.store.db, **plan['database']) if plan['database'] is not None else plan['value']

    def view(self, session, *, query=None, heartbeat=False):
        return self.queue_io(self.core._call('api_view_begin', generation=session.generation,
            query={} if query is None else query, heartbeat=heartbeat, rooms=self.queue_room_facts()), session)

    async def poll(self, request):
        _, session = self.session(request)
        return web.json_response(self.view(session, query=self.query_facts(request), heartbeat=True))

    async def cancel(self, request):
        _, session = self.session(request)
        result = self.core._call('api_cancel', generation=session.generation,
            query=self.query_facts(request), rooms=self.queue_room_facts())
        return web.json_response(result)

    async def content_failure(self, request):
        body = await self.directory.body(request)
        _, session = self.session(request)
        effect = self.core._call('api_content_failure', generation=session.generation,
            body=body, rooms=self.queue_room_facts())
        if effect['close'] is not None:
            room = self.transport.rooms.get(effect['close'])
            if room:
                await room.close(effect['reason'])
        return web.json_response(effect['response'])

    def routes(self):
        return [web.post("/v1/matchmaking/session", self.create_session),
                web.delete("/v1/matchmaking/session", self.delete_session),
                web.get("/v1/matchmaking/playlists", self.playlists),
                web.post("/v1/matchmaking/ticket", self.enqueue),
                web.get("/v1/matchmaking/ticket", self.poll),
                web.post("/v1/matchmaking/failure", self.content_failure),
                web.delete("/v1/matchmaking/ticket", self.cancel)]
