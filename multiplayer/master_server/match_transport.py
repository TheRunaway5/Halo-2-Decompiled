"""Authenticated native match relay; not the retail Xbox wire protocol.

The service observes challenge round trips itself. Clients cannot supply QoS,
rank, a host, or another player's identity. Gameplay remains host authoritative.
"""
import asyncio
import contextlib
import json
import logging
import secrets
import statistics
import sqlite3
import time
from aiohttp import web, WSMsgType
from .protocol import require, Problem


class Room:
    def __init__(self, api, proposal):
        self.api, self.id, self.proposal = api, proposal.id, proposal
        names = {p.id: p.screen_name for t in proposal.tickets for p in api.core.tickets[t].players}
        self.players = [(p, names[p], team) for team, players in enumerate(proposal.teams) for p in players]
        self.tokens = {p: secrets.token_urlsafe(32) for p, _, _ in self.players}
        # Native per-room presentation identities; never Discord snowflakes or
        # persistent account IDs. These are not retail Xbox Live XUIDs.
        self.identities = {p: secrets.token_hex(8) for p, _, _ in self.players}
        self.ranked = api.core.playlists[proposal.playlist].ranked
        self.team_mode = bool(api.core.playlists[proposal.playlist].team_size)
        configuration = next(e.rank_configuration for e in api.entries if e.policy.id == proposal.playlist)
        self.clans = {}
        if api.core.playlists[proposal.playlist].clan_match:
            for team, players in enumerate(proposal.teams):
                self.clans[team] = api.common_clan(players, api.core.playlists[proposal.playlist])
            require(len(self.clans) == 2 and len(set(self.clans.values())) == 2, "Clan match owners changed", 409)
        self.clan_configuration = configuration if self.clans else None
        self.clan_skills = {clan: api.clan_skills.snapshot(clan, proposal.playlist, configuration)
                            for clan in self.clans.values()}
        self.skill_configuration = None if self.clans else configuration
        self.skills = {p: api.skills.snapshot(p, proposal.playlist, self.skill_configuration)
                       for p, _, _ in self.players} if self.skill_configuration else {}
        self.result, self.result_digest, self.confirmations = b"", "", set()
        self.peers, self.samples, self.loaded = {}, {}, {}
        self.phase, self.host = "connecting", None
        self.created, self.deadline = time.monotonic(), time.monotonic() + 25
        self.closed = False

    def descriptor(self, player):
        return {"id": self.id, "token": self.tokens[player], "slot": self.index(player),
                "players": [{"name": name, "team": team, "identity": self.identities[p]}
                            for p, name, team in self.players]}

    def index(self, player):
        return next(i for i, row in enumerate(self.players) if row[0] == player)

    def control(self):
        return json.dumps({"phase": self.phase, "host": self.host})

    def voice_membership(self):
        return json.dumps({"voice_host": self.host, "voice_members": sum(1 << self.index(p) for p in self.peers)})

    def voice_block_mask(self, player):
        blocks = self.api.directory.voice_blocks
        return sum(1 << i for i, (other, _, _) in enumerate(self.players)
                   if other != player and not blocks.allowed(player, other))

    async def close(self, reason="A player disconnected. Search again."):
        if self.closed:
            return
        self.closed = True
        for ticket in self.proposal.tickets:
            if ticket in self.api.core.tickets:
                self.api.core.cancel(ticket)
        await asyncio.gather(*(p.socket.close(code=1000, message=reason.encode()[:120])
                               for p in tuple(self.peers.values())), return_exceptions=True)

    async def advance(self):
        if self.closed:
            return
        if self.phase == "connecting" and all(len(self.samples.get(p, ())) >= 3 for p, _, _ in self.players):
            # Explicit native relay score, not an assertion about Xbox QoS.
            for a, _, _ in self.players:
                for b, _, _ in self.players:
                    if a < b:
                        ms = statistics.median(self.samples[a]) + statistics.median(self.samples[b])
                        score = max(0, 100 - int(ms / 5))
                        self.api.core.connection(a, b, score)
                        self.api.core.connection(b, a, score)
            proposal = self.api.core.refresh_connections(self.id)
            if proposal.host is None:
                await self.close("Connection quality is insufficient for this playlist.")
                return
            self.host = self.index(proposal.host)
            for player, _, _ in self.players:
                self.api.core.acknowledge(self.id, player)
            self.phase, self.deadline = "loading", time.monotonic() + 180
            for peer in self.peers.values():
                peer.queue.put_nowait(self.control())
        if self.phase == "loading" and len(self.loaded) == len(self.players):
            if len(set(self.loaded.values())) != 1:
                await self.close("Map, game data, or native protocol differs between players.")
                return
            self.phase, self.deadline = "running", time.monotonic() + 7200
            for peer in self.peers.values():
                peer.queue.put_nowait(self.control())


class PacketBudget:
    """Native abuse limits, not recovered Xbox QoS or voice coefficients.

    Authenticated route/roster validators bound the possible stream keys. The
    native20ms codec produces50 packets/s per speaker/destination plus5/s
    postgame preferences; its75/s ceiling includes jitter headroom without
    charging host fanout to one mic.
    Game datagrams retain512-burst/1000-per-second caps per validated route:
    a32KiB snapshot is up to24 fragments per recipient at30Hz. Control and
    ignored postgame game packets have their own512/1000 shared cap.
    """
    def __init__(self, *, clock=time.monotonic):
        self.clock, self.streams = clock, {}

    def take(self, audio=None, *, game=None):
        now = self.clock()
        rate, burst = (75.,75.) if audio is not None else (1000.,512.)
        key = ("audio",*audio) if audio is not None else (("game",game) if game is not None else None)
        credits, previous = self.streams.get(key, (burst,now))
        credits = min(burst,credits+(now-previous)*rate)
        require(credits >= 1, "Match packet rate exceeded", 429)
        self.streams[key] = credits-1, now


class Peer:
    def __init__(self, socket):
        self.socket, self.queue = socket, asyncio.Queue(maxsize=512)
        self.challenge, self.sent = "", 0.
        self.budget = PacketBudget()


class MatchTransport:
    def __init__(self, api):
        self.api, self.rooms = api, {}
        api.transport = self

    async def cleanup(self):
        for room in tuple(self.rooms.values()):
            if not room.closed and (time.monotonic() > room.deadline or
                                    (room.phase == "connecting" and room.id not in self.api.core.proposals)):
                await room.close("Match connection or loading timed out. Search again.")
        self.rooms = {k: v for k, v in self.rooms.items() if not v.closed}

    async def descriptor(self, request):
        await self.cleanup()
        _, session = self.api.session(request)
        ticket = self.api.ticket(session.player)
        require(ticket is not None, "No active match search", 409)
        self.api.advance(ticket.playlist)
        status = self.api.core.status(ticket.id)
        require("proposal_id" in status, "Players are still being found", 409)
        ident = status["proposal_id"]
        # Expired rooms hold no credentials after all readers are gone.
        self.rooms = {k: v for k, v in self.rooms.items() if not v.closed}
        require(len(self.rooms) < 64 or ident in self.rooms, "Match capacity reached", 503)
        if ident not in self.rooms:
            self.rooms[ident] = Room(self.api, self.api.core.proposals[ident])
        room = self.rooms[ident]
        return web.json_response(room.descriptor(session.player))

    async def writer(self, room, player, peer):
        blocked = 0
        while not room.closed:
            mask = room.voice_block_mask(player)
            if mask != blocked:
                try:
                    await asyncio.wait_for(peer.socket.send_str(json.dumps({"voice_blocked": mask})), .5)
                except (ConnectionError, asyncio.TimeoutError):
                    await room.close()
                    return
                blocked = mask
            if time.monotonic() > room.deadline or (room.phase == "connecting" and room.id not in self.api.core.proposals):
                await room.close("Match connection or loading timed out. Search again.")
                return
            if not peer.challenge and (time.monotonic() - peer.sent > (.2 if room.phase == "connecting" else 5)):
                peer.challenge, peer.sent = secrets.token_hex(8), time.monotonic()
                await peer.socket.send_str(json.dumps({"ping": peer.challenge}))
            elif peer.challenge and time.monotonic() - peer.sent > 15:
                await room.close()
                return
            try:
                packet = await asyncio.wait_for(peer.queue.get(), .05)
            except asyncio.TimeoutError:
                continue
            try:
                if isinstance(packet, str):
                    await asyncio.wait_for(peer.socket.send_str(packet), .5)
                else:
                    # The host must receive client microphones for onward
                    # routing, even when its own playback is blocked. Native
                    # host playback obeys the separate server-owned mask.
                    if (room.index(player) != room.host and len(packet) >= 10
                            and packet[7:9] == b"\x06\x00"
                            and not self.api.directory.voice_blocks.allowed(room.players[packet[9]][0], player)):
                        continue
                    await asyncio.wait_for(peer.socket.send_bytes(packet), .5)
            except (ConnectionError, asyncio.TimeoutError):
                await room.close()
                return

    async def connect(self, request):
        room = self.rooms.get(request.match_info["room"])
        require(room is not None and not room.closed, "Match no longer exists", 404)
        bearer = request.headers.get("Authorization", "")
        require(bearer.startswith("Bearer ") and len(bearer) < 128, "Match authorization required", 401)
        player = next((p for p, token in room.tokens.items() if secrets.compare_digest(token, bearer[7:])), None)
        require(player is not None, "Match authorization required", 401)
        require(player not in room.peers, "Already connected to match", 409)
        # aiohttp's parser rejects messages whose size equals this ceiling.
        # A complete 1400-byte native datagram plus its route byte is valid.
        socket = web.WebSocketResponse(max_msg_size=120000, compress=False)
        await socket.prepare(request)
        if room.closed or player in room.peers:
            await socket.close(code=1008)
            return socket
        peer = Peer(socket)
        room.peers[player] = peer
        peer.queue.put_nowait(room.control())
        writer = asyncio.create_task(self.writer(room, player, peer))
        try:
            async for message in socket:
                now = time.monotonic()
                if message.type == WSMsgType.TEXT:
                    peer.budget.take()
                    require(len(message.data) <= 119000, "Invalid match control")
                    data = json.loads(message.data)
                    if set(data) == {"pong"}:
                        require(peer.challenge and data["pong"] == peer.challenge, "Invalid connection challenge")
                        room.samples.setdefault(player, []).append((now - peer.sent) * 1000)
                        room.samples[player] = room.samples[player][-3:]
                        peer.challenge = ""
                    elif set(data) == {"loaded"}:
                        require(room.phase == "loading" and isinstance(data["loaded"], str)
                                and len(data["loaded"]) == 24 and all(c in "0123456789abcdef:" for c in data["loaded"]),
                                "Invalid game-data admission")
                        room.loaded[player] = data["loaded"]
                    elif set(data) == {"postgame"}:
                        report = data["postgame"]
                        require(room.phase == "running" and room.index(player) == room.host
                                and len(room.peers) == len(room.players), "Invalid postgame report")
                        room.result, room.result_digest = self.api.results.validate(room, report)
                        room.phase, room.deadline = "reviewing", now + 30
                        publication = json.dumps({"postgame": report})
                        for recipient in room.peers.values():
                            recipient.queue.put_nowait(publication)
                    elif set(data) == {"confirm_result"}:
                        require(room.phase in ("reviewing", "postgame") and
                                isinstance(data["confirm_result"], str) and
                                secrets.compare_digest(data["confirm_result"], room.result_digest),
                                "Match results disagree; no competitive result was recorded")
                        room.confirmations.add(player)
                        if room.phase == "postgame" or len(room.confirmations) != len(room.players):
                            continue
                        self.api.results.commit(room)
                        room.phase, room.deadline = "postgame", now + 900
                        for ticket in room.proposal.tickets:
                            if ticket in self.api.core.tickets:
                                self.api.core.cancel(ticket)
                        publication = json.dumps({"completed": room.result_digest})
                        for recipient in room.peers.values():
                            recipient.queue.put_nowait(publication)
                            recipient.queue.put_nowait(room.voice_membership())
                    else:
                        require(False, "Unknown match control")
                    await room.advance()
                elif message.type == WSMsgType.BINARY:
                    packet = message.data
                    source = room.index(player)
                    require(room.phase in ("running", "reviewing", "postgame") and 8 <= len(packet) <= 1401, "Invalid game packet")
                    target, native = packet[0], packet[1:]
                    require(native[:4] == b"H2N2", "Invalid native packet")
                    kind = native[6]
                    # Completed simulation traffic may already be in flight;
                    # only authenticated session voice survives world teardown.
                    if room.phase in ("reviewing", "postgame") and kind != 6:
                        peer.budget.take()
                        continue
                    if kind == 6:
                        require(len(native) >= 11, "Invalid voice packet")
                        subtype = native[7]
                        require(subtype in (0, 1, 2), "Invalid voice control")
                        if subtype == 0:
                            require(22 < len(native) <= 278 and native[8] < len(room.players)
                                    and (source == room.host or native[8] == source), "Invalid voice identity")
                        else:
                            require(len(native) == (11 if subtype == 1 else 12) and native[8] <= 1,
                                    "Invalid voice preferences")
                            if subtype == 2:
                                require(source == room.host and native[11] < len(room.players), "Invalid voice membership")
                        if room.phase == "postgame" and source != room.host:
                            target = room.host  # A completed session's former voice host may have left.
                    if source == room.host:
                        require(target != source and target < len(room.players) and kind in (3, 5, 6), "Invalid host route")
                    else:
                        require(target == room.host and kind in (1, 2, 4, 6), "Invalid client route")
                    if kind == 6:
                        speaker = native[8] if native[7] == 0 else (source if native[7] == 1 else native[11])
                        peer.budget.take((speaker,target))
                        if (native[7] == 0 and source == room.host
                                and not self.api.directory.voice_blocks.allowed(room.players[speaker][0], room.players[target][0])):
                            continue
                    else:
                        peer.budget.take(game=target)
                    destination = room.peers.get(room.players[target][0])
                    if destination is None and room.phase == "postgame":
                        continue
                    require(destination is not None and not destination.queue.full(), "Match reader stalled", 503)
                    destination.queue.put_nowait(bytes([source]) + native)
                else:
                    break
        except Problem as error:
            logging.getLogger("halo2.match").warning("Match relay closed: %s", error.message)
            await room.close(error.message)
        except sqlite3.Error:
            logging.getLogger("halo2.match").error("Unable to save confirmed match result")
            await room.close("Unable to save the completed match. Search again.")
        except (web.HTTPException, ValueError, KeyError, TypeError, asyncio.QueueFull):
            pass
        finally:
            writer.cancel()
            with contextlib.suppress(asyncio.CancelledError):
                await writer
            if room.phase == "postgame":
                room.peers.pop(player, None)
                if not room.peers:
                    await room.close("Match completed.")
                else:
                    room.host = min(room.index(p) for p in room.peers) if room.players[room.host][0] not in room.peers else room.host
                    for remaining in room.peers.values():
                        if not remaining.queue.full():
                            remaining.queue.put_nowait(room.voice_membership())
            else:
                await room.close()
        return socket

    async def close(self, app):
        await asyncio.gather(*(r.close("Match service is restarting.") for r in tuple(self.rooms.values())))

    def routes(self):
        return [web.get("/v1/matchmaking/transport", self.descriptor),
                web.get("/v1/matches/{room}/relay", self.connect)]
