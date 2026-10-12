"""Discord-authorized native queue API. No client-supplied identities or ranks."""
from dataclasses import dataclass
from fractions import Fraction
import hashlib
from pathlib import Path
import secrets
import time
import uuid

from aiohttp import web

from .matchmaking import Coordinator, Game, Player, Playlist, SearchStage, PARTY_RULES_2007, CLAN_RULES_RETAIL
from .protocol import fields, identifier, integer, parse, require, text, Problem
from .player_identity import initialize, player as resolve_player, VERIFIED_DISCORD_USERNAME
from .playlist_skill import PlaylistSkills, POLICY, display_level
from .clan_skill import ClanSkills
from .search_lifecycle import GatheringConfiguration, retail_configuration


@dataclass(frozen=True)
class CatalogEntry:
    policy: Playlist
    name: str
    description: str
    quickmatch: bool
    rank_configuration: str | None = None


def load_catalog(path):
    if not path:
        return ()
    raw = Path(path).read_bytes()
    require(len(raw) <= 1024 * 1024, "Matchmaking catalog too large")
    root = parse(raw)
    fields(root, ("version", "playlists"))
    require(type(root["version"]) is int and root["version"] == 1, "Unsupported matchmaking catalog")
    require(type(root["playlists"]) is list and 1 <= len(root["playlists"]) <= 64, "Invalid playlist catalog")
    entries = []
    for row in root["playlists"]:
        fields(row, ("id", "revision", "name", "description", "ranked", "quickmatch", "team_size",
                     "minimum_players", "maximum_players", "maximum_party", "games", "stages", "skill", "provenance"),
               ("party_balance", "rank_configuration", "clan_match", "candidate_selection", "gathering"))
        require(type(row["quickmatch"]) is bool, "Invalid Quickmatch flag")
        require(type(row["games"]) is list and 1 <= len(row["games"]) <= 256, "Invalid game pool")
        games = []
        for game in row["games"]:
            fields(game, ("id", "map", "variant", "weight"))
            games.append(Game(**game))
        require(type(row["stages"]) is list and 1 <= len(row["stages"]) <= 16, "Invalid search stages")
        stages = []
        for stage in row["stages"]:
            fields(stage, ("after_seconds", "rank_numerator", "rank_denominator", "minimum_connection"),
                   ("unbounded_skill",))
            numerator = integer(stage["rank_numerator"], 1, 10000)
            denominator = integer(stage["rank_denominator"], 1, 10000)
            stages.append(SearchStage(stage["after_seconds"], Fraction(numerator, denominator),
                                      stage["minimum_connection"], stage.get("unbounded_skill", False)))
        skill = row["skill"]
        fields(skill, ("kind",), ("radius", "high_rank_radius", "mixed_party_floor"))
        require(skill["kind"] in ("bungie-2007", "custom"), "Invalid skill policy")
        historical = skill["kind"] == "bungie-2007"
        require((historical and "radius" not in skill) or
                (not historical and set(skill) == {"kind", "radius"}), "Incompatible skill fields")
        balance = row.get("party_balance")
        require(balance is None or balance in PARTY_RULES_2007, "Unknown party balance policy")
        clan_match = row.get("clan_match")
        require(clan_match is None or clan_match in CLAN_RULES_RETAIL, "Unknown clan lineup policy")
        gathering = row.get("gathering")
        if gathering is not None:
            if type(gathering) is dict and set(gathering) == {"retail_hopper"}:
                minimum,maximum,gathering = retail_configuration(integer(gathering['retail_hopper'],0,6))
                require((row['minimum_players'],row['maximum_players']) == (minimum,maximum),
                        "Retail gathering dimensions differ from this playlist")
            else:
                fields(gathering, ("countdown_ms", "timeout_ms", "provenance"))
                require(type(gathering["countdown_ms"]) is list and len(gathering["countdown_ms"]) == 16,
                        "Gathering requires all sixteen countdown entries")
                gathering = GatheringConfiguration(tuple(integer(n,0,0x7fffffff) for n in gathering["countdown_ms"]),
                    integer(gathering["timeout_ms"],10000,3600000),text(gathering["provenance"],512))
        policy = Playlist(text(row["id"]), text(row["revision"]), row["ranked"], row["team_size"],
                          row["minimum_players"], row["maximum_players"], row["maximum_party"],
                          tuple(games), tuple(stages), row["provenance"],
                          PARTY_RULES_2007.get(balance), historical, skill.get("high_rank_radius"),
                          skill.get("mixed_party_floor", False), skill.get("radius"), CLAN_RULES_RETAIL.get(clan_match),
                          row.get("candidate_selection"), gathering)
        configuration = row.get("rank_configuration")
        require(configuration is None or configuration == POLICY, "Unknown rank configuration")
        require(clan_match is None or configuration == POLICY, "Clan playlists require a supported clan rank configuration")
        # Retail types1/3 update the same per-hopper XP while hiding its level;
        # types2/4 expose it. Visibility is independent of the versioned tables.
        entries.append(CatalogEntry(policy, text(row["name"]), text(row["description"], 256),
                                    row["quickmatch"], configuration))
    require(len({e.policy.id for e in entries}) == len(entries), "Duplicate playlist")
    require(sum(e.quickmatch for e in entries) <= 1, "Only one Quickmatch default is allowed")
    return tuple(entries)


@dataclass
class Session:
    player: str
    screen_name: str
    maps: frozenset[str]
    compatibility: str
    expires: float
    seen: float
    language: int = 0  # Native title-language index, never a Discord locale.


class MatchmakingApi:
    def __init__(self, directory, entries=(), *, clock=time.monotonic):
        self.directory, self.entries, self.clock = directory, entries, clock
        self.core = Coordinator(tuple(e.policy for e in entries), clock=clock) if entries else None
        self.sessions, self.requests = {}, {}
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

    def enabled(self):
        require(self.core is not None, "Matchmaking playlists are not configured", 503)

    def ticket(self, player):
        return next((t for t in self.core.tickets.values() if any(p.id == player for p in t.players)), None)

    def cleanup(self):
        if not self.core:
            return
        self.core.cleanup()
        for ticket in tuple(self.core.tickets.values()):
            if ticket.clan and any(self.member_clan(p.id) != ticket.clan for p in ticket.players):
                self.core.cancel(ticket.id)
        for key, session in list(self.sessions.items()):
            if session.expires <= self.clock():
                self.remove_session(key)

    def remove_session(self, key):
        session = self.sessions.pop(key, None)
        if session:
            qos = getattr(self, "search_qos", None)
            if qos:
                qos.discard(session.player)
            ticket = self.ticket(session.player)
            if ticket:
                self.core.cancel(ticket.id)
            gatherings = getattr(self, "gatherings", None)
            if gatherings:
                gatherings.cleanup()
            self.requests = {k: v for k, v in self.requests.items() if k[0] != session.player}
            parties = getattr(self.directory, "parties", None)
            if parties: parties.session_lost(session.player)

    def session(self, request):
        self.enabled()
        self.cleanup()
        header = request.headers.get("Authorization", "")
        require(header.startswith("Bearer ") and len(header) <= 256, "Matchmaking session required", 401)
        key = hashlib.sha256(header[7:].encode()).hexdigest()
        value = self.sessions.get(key)
        require(value is not None, "Matchmaking session expired; reconnect", 401)
        value.seen = self.clock()
        return key, value

    async def create_session(self, request):
        self.enabled()
        body = await self.directory.body(request)
        fields(body, ("screen_name", "maps", "compatibility"), ("language",))
        name, compatibility = text(body["screen_name"]), text(body["compatibility"])
        require(not compatibility.startswith("halo2-native-queue-") or compatibility in
                ("halo2-native-queue-v7", "halo2-native-queue-v8"),
                "Update Halo 2 before using ONLINE matchmaking", 409)
        require(compatibility != "halo2-native-queue-v8" or "language" in body,
                "Native title language required", 400)
        language = integer(body.get("language", 0), 0, 8)
        try:
            name_units = name.encode("utf-16-le")
        except UnicodeError:
            raise Problem(400, "Screen name contains invalid Unicode") from None
        require(len(name_units) <= 62, "Screen name must fit 31 UTF-16 characters")
        require(type(body["maps"]) is list and 1 <= len(body["maps"]) <= 256, "Invalid map capabilities")
        maps = frozenset(text(value) for value in body["maps"])
        require(len(maps) == len(body["maps"]), "Duplicate map capability")
        provider, subject = await self.directory.owner(request)
        self.cleanup()
        db = self.directory.store.db
        player = resolve_player(db, (provider, subject), name, request.get(VERIFIED_DISCORD_USERNAME, ""))
        for key, old in list(self.sessions.items()):
            if old.player == player:
                self.remove_session(key)
        require(len(self.sessions) < 1000, "Session capacity reached", 503)
        token = secrets.token_urlsafe(32)
        self.sessions[hashlib.sha256(token.encode()).hexdigest()] = Session(player, name, maps, compatibility,
                                                                        self.clock() + 900, self.clock(), language)
        return web.json_response({"token": token, "expires_in": 900}, status=201)

    async def delete_session(self, request):
        key, _ = self.session(request)
        self.remove_session(key)
        return web.json_response({"state": "signed_out"})

    def rank(self, player, playlist):
        entry = next(e for e in self.entries if e.policy.id == playlist)
        if entry.policy.clan_match:
            clan = self.member_clan(player)
            return display_level(self.clan_rank(clan, entry)) if clan else None
        if entry.rank_configuration:
            return display_level(self.skills.snapshot(player, playlist, entry.rank_configuration).raw)
        db = self.directory.store.db
        with db:
            db.execute("INSERT OR IGNORE INTO playlist_ranks VALUES(?,?,1)", (player, playlist))
            return db.execute("SELECT rank FROM playlist_ranks WHERE player=? AND playlist=?", (player, playlist)).fetchone()[0]

    def member_clan(self, player):
        member = self.directory.clans.member(player)
        return member[0] if member else None

    def clan_rank(self, clan, entry):
        # Search retains the full private raw range, including ranks sharing
        # the final visible artwork. Display level never replaces clan skill.
        return self.clan_skills.snapshot(clan, entry.policy.id, entry.rank_configuration).raw

    def common_clan(self, players, policy):
        rule = policy.clan_match
        require(rule.minimum <= len(players) <= rule.maximum,
                f"This clan playlist requires {rule.minimum}–{rule.maximum} players in one party", 409)
        clans = {self.member_clan(player) for player in players}
        require(len(clans) == 1 and None not in clans, "Every party member must belong to the same accepted clan", 409)
        return next(iter(clans))

    def clan_membership_changing(self, player):
        """Native authority policy: preserve admitted owners, revoke queued consent."""
        transport = getattr(self, "transport", None)
        require(transport is None or not any(player in r.tokens and r.clans and not r.closed
                and r.phase != "postgame" for r in transport.rooms.values()), "Finish your clan match first", 409)
        ticket = self.ticket(player) if self.core else None
        if ticket and ticket.clan:
            self.directory.parties.changed(ticket.party_id)

    def eligibility(self, session, policy):
        if policy.candidate_selection and session.compatibility != "halo2-native-queue-v8":
            return False
        parties = getattr(self.directory, "parties", None)
        party = parties.party(session.player) if parties else None
        if policy.clan_match:
            players = [r[0] for r in parties.members(party[0])] if party else [session.player]
            clans = {self.member_clan(player) for player in players}
            if (not policy.clan_match.minimum <= len(players) <= policy.clan_match.maximum or
                    len(clans) != 1 or None in clans):
                return False
        return (all(game.map in session.maps for game in policy.games)
                and (party is None or len(parties.members(party[0])) <= policy.maximum_party))

    async def playlists(self, request):
        _, session = self.session(request)
        rows = []
        for entry in self.entries:
            policy = entry.policy
            rows.append({"id": policy.id, "revision": policy.revision, "name": entry.name,
                         "description": entry.description, "ranked": policy.ranked, "quickmatch": entry.quickmatch,
                         "minimum_players": policy.minimum_players, "maximum_players": policy.maximum_players,
                         "eligible": self.eligibility(session, policy),
                         "level": self.rank(session.player, policy.id) if policy.ranked else None})
            if policy.clan_match:
                rows[-1]["clan_match"] = True
        return web.json_response({"playlists": rows})

    async def enqueue(self, request):
        # Parse before session lookup: no await is allowed between authorization
        # and mutation, including when a simultaneous DELETE revokes a session.
        body = await self.directory.body(request)
        fields(body, ("playlist_id", "revision", "request_id"))
        request_id = identifier(body["request_id"])
        _, session = self.session(request)
        policy = self.core.playlists.get(text(body["playlist_id"]))
        transport = getattr(self, "transport", None)
        require(transport is None or not any(session.player in room.tokens and not room.closed
                and room.phase not in ("connecting", "postgame") for room in transport.rooms.values()),
                "Leave the current ONLINE match before searching again", 409)
        require(policy is not None, "Playlist not found", 404)
        require(policy.revision == text(body["revision"]), "Playlist changed; refresh playlists", 409)
        key = session.player, request_id
        if key in self.requests:
            ticket = self.core.tickets.get(self.requests[key])
            require(ticket is not None and ticket.playlist == policy.id, "Queue request already completed or cancelled", 409)
        else:
            require(len([k for k in self.requests if k[0] == session.player]) < 64, "Reconnect before more queue requests", 429)
            current = self.ticket(session.player)
            require(current is None or current.playlist == policy.id, "Already searching another playlist", 409)
            party_id, members = self.directory.parties.queue_members(session.player, policy)
            require(not policy.candidate_selection or all(member.compatibility == "halo2-native-queue-v8" for member in members),
                    "Every party member must update Halo 2 for gathering matchmaking", 409)
            clan, clan_rank = None, None
            if policy.clan_match:
                clan = self.common_clan([member.player for member in members], policy)
                require(transport is None or not any(clan in room.clans.values() and room.proposal.playlist == policy.id
                        and not room.closed and room.phase != "postgame" for room in transport.rooms.values()),
                        "This clan is already playing this playlist", 409)
                entry = next(e for e in self.entries if e.policy.id == policy.id)
                clan_rank = self.clan_rank(clan, entry)
            entry = next(e for e in self.entries if e.policy.id == policy.id)
            players = []
            for member in members:
                raw = (self.skills.snapshot(member.player, policy.id, entry.rank_configuration).raw
                       if entry.rank_configuration and not clan else None)
                players.append(Player(member.player, member.screen_name,
                                      display_level(raw) if raw is not None else
                                      (1 if clan else self.rank(member.player, policy.id)), member.maps, raw))
            ticket = self.core.enqueue(party_id, policy.id, session.compatibility,
                                       tuple(players), clan=clan, clan_rank=clan_rank,
                                       leader=session.player, language=session.language)
            self.requests[key] = ticket.id
        return web.json_response(self.view(session), status=201)

    def advance(self, playlist):
        # Grouping is useful before a native connectivity adapter exists. A
        # hostless proposal cannot pass readiness or become a gameplay handoff.
        self.cleanup()
        if self.core.playlists[playlist].candidate_selection:
            self.gatherings.advance(playlist)
        else:
            self.core.advance(playlist, allow_pending_connectivity=True)

    def view(self, session):
        ticket = self.ticket(session.player)
        if not ticket:
            return {"state": "idle", "playlist_id": "", "queued_players": 0, "match": None}
        self.advance(ticket.playlist)
        status = self.core.status(ticket.id)
        match = None
        if "proposal_id" in status:
            proposal = self.core.refresh_connections(status["proposal_id"])
            status = self.core.status(ticket.id)
            roster = {p.id: p.screen_name for key in proposal.tickets for p in self.core.tickets[key].players}
            match = {"map": proposal.game.map, "variant": proposal.game.variant,
                     "players": [{"name": roster[player], "team": team, "self": player == session.player}
                                 for team, players in enumerate(proposal.teams) for player in players]}
        count = sum(len(t.players) for t in self.core.tickets.values()
                    if t.playlist == ticket.playlist and t.compatibility == ticket.compatibility)
        return {"state": status["state"], "playlist_id": ticket.playlist, "queued_players": count, "match": match}

    async def poll(self, request):
        _, session = self.session(request)
        ticket = self.ticket(session.player)
        if ticket:
            self.core.heartbeat(ticket.id)
        result = self.view(session)
        # Existing native v8 clients validate the exact four-field response.
        # Updated UIs opt in without changing queue/gameplay compatibility.
        if request.query.get("progress") == "1":
            result["progress"] = (self.gatherings.progress(self.ticket(session.player),session.player)
                                  if result["state"] == "searching" else None)
        return web.json_response(result)

    async def cancel(self, request):
        _, session = self.session(request)
        ticket = self.ticket(session.player)
        if ticket:
            self.core.cancel(ticket.id)
        self.gatherings.cleanup()
        return web.json_response({"state": "idle", "playlist_id": "", "queued_players": 0, "match": None})

    def routes(self):
        return [web.post("/v1/matchmaking/session", self.create_session),
                web.delete("/v1/matchmaking/session", self.delete_session),
                web.get("/v1/matchmaking/playlists", self.playlists),
                web.post("/v1/matchmaking/ticket", self.enqueue),
                web.get("/v1/matchmaking/ticket", self.poll),
                web.delete("/v1/matchmaking/ticket", self.cancel)]
