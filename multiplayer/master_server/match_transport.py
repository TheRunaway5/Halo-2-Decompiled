"""Authenticated native match setup and legacy relay, not Xbox wire protocol.

Direct rooms exchange gameplay/media only between players. The coordinator
receives admitted peer measurement summaries for setup, never probe payloads.
These authenticated reports are client claims, not anti-cheat attestations.
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
from .couch_players import NATIVE_COUCH, NATIVE_CONTINUITY, NATIVE_DIRECT, NATIVE_EOS
from .match_continuity import PlayerHostedContinuity
from .voice_origin import Ed25519PrivateKey, verify_voice


class Room:
    def __init__(self, api, proposal):
        self.api, self.id, self.proposal = api, proposal.id, proposal
        names = {p.id: p.screen_name for t in proposal.tickets for p in api.core.tickets[t].players}
        admitted = {p.id: p for t in proposal.tickets for p in api.core.tickets[t].players}
        self.owners = {p.id: p.connection_owner or p.id for p in admitted.values()}
        self.guests = frozenset(p.id for p in admitted.values() if p.guest)
        self.compatibility = api.core.tickets[proposal.tickets[0]].compatibility
        self.couch_protocol = self.compatibility in NATIVE_COUCH
        self.supports_continuity = self.compatibility == NATIVE_CONTINUITY
        self.eos = self.compatibility == NATIVE_EOS
        self.direct_udp = self.compatibility in (NATIVE_DIRECT, NATIVE_EOS)
        self.eos_socket, self.eos_users = secrets.token_hex(16), {}
        physical = sorted(set(self.owners.values()))
        self.eos_keys = {(a, b): secrets.token_hex(32) for i, a in enumerate(physical) for b in physical[i+1:]} if self.eos else {}
        self.direct_ready, self.pending_signals, self.signaled_descriptions = {}, {}, set()
        self.signal_counts = {}
        self.players = [(p, names[p], team) for team, players in enumerate(proposal.teams) for p in players]
        self.tokens = {self.owners[p]: secrets.token_urlsafe(32) for p, _, _ in self.players}
        self.admitted_owners = frozenset(self.tokens)
        self.voice_keys = {owner: Ed25519PrivateKey.generate() for owner in self.admitted_owners}
        self.departed_slots = 0
        self.continuity = None
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
                       for p, _, _ in self.players if p not in self.guests} if self.skill_configuration else {}
        self.result, self.result_digest, self.confirmations = b"", "", set()
        self.peers, self.samples, self.loaded = {}, {}, {}
        self.phase, self.host = "connecting", None
        self.created, self.deadline = time.monotonic(), time.monotonic() + (60 if self.eos else 25)
        self.created_wall = api.directory.store.now()
        self.closed = False

    def descriptor(self, player):
        result = {"id": self.id, "token": self.tokens[player], "slot": self.index(player),
                "players": [{"name": name, "team": team, "identity": self.identities[p]}
                            for p, name, team in self.players]}
        if self.compatibility in ("halo2-native-queue-v10", NATIVE_CONTINUITY, NATIVE_DIRECT, NATIVE_EOS):
            result["team_mode"] = self.team_mode
        if self.direct_udp:
            result["transport"] = {"kind": "direct-udp-v1", "stun_servers": list(self.api.directory.settings.peer_stun_urls)}
        if self.eos:
            result["transport"] = {"kind": "eos-p2p-v1", "socket": self.eos_socket,
                "peers": [{"slot": self.index(b if a == player else a), "key": key}
                          for (a, b), key in self.eos_keys.items() if player in (a, b)],
                "session": {"playlist": self.proposal.playlist, "revision": self.proposal.revision,
                            "map": self.proposal.game.map, "variant": self.proposal.game.variant}}
        if self.supports_continuity:
            result["continuity"] = "complete-world-v1"
        if self.couch_protocol:
            result["local_slots"] = [i for i, (p, _, _) in enumerate(self.players) if self.owners[p] == player]
            for row, (p, _, _) in zip(result["players"], self.players):
                row.update(peer=self.index(self.owners[p]), guest=p in self.guests)
        return result

    def index(self, player):
        return next(i for i, row in enumerate(self.players) if row[0] == player)

    def active_member(self, player):
        return player in self.owners and self.owners[player] in self.tokens

    def voice_auth(self, player):
        # Only this authenticated connection receives its signing seed. Public
        # keys identify physical owners, including their admitted couch seats.
        return json.dumps({'voice_auth': {'version': 1,
            'seed': self.voice_keys[player].private_bytes_raw().hex(),
            'keys': [self.voice_keys[self.owners[p]].public_key().public_bytes_raw().hex()
                     for p, _, _ in self.players]}})

    def control(self):
        value = {"phase": self.phase, "host": self.host}
        if self.supports_continuity:
            value["continuity"] = self.continuity.control() if self.continuity else None
            if self.continuity:
                value["host"] = value["continuity"]["host"]
        return json.dumps(value)

    def broadcast(self, packet):
        for peer in tuple(self.peers.values()):
            if peer.queue.full():
                # This reader's teardown removes its own connection. It must
                # not terminate otherwise healthy readers' active match.
                asyncio.create_task(peer.socket.close(code=1008, message=b"Match reader stalled"))
            else:
                peer.queue.put_nowait(packet)

    async def synchronize_continuity(self):
        if not self.continuity or self.closed:
            return
        self.continuity.expire()
        for player in tuple(self.peers):
            if self.index(player) not in self.continuity.active:
                peer = self.peers.pop(player)
                self.retire_capability(player)
                asyncio.create_task(peer.socket.close(code=1008, message=b"Match rebind timed out"))
        if self.continuity.phase in ("empty", "unavailable"):
            await self.close("No surviving peer has the required complete world.")
            return
        self.host, self.phase = self.continuity.host, self.continuity.phase
        self.broadcast(self.control())

    def retire_capability(self, player):
        self.tokens.pop(player, None)
        # Transport retirement neither revokes an authenticated digest vote
        # nor removes its owner from the immutable result electorate.
        self.pending_signals.pop(player, None)
        self.departed_slots |= sum(1 << i for i, (p, _, _) in enumerate(self.players)
                                   if self.owners[p] == player)

    async def depart_direct(self, player):
        """Explicit authenticated leave; TCP loss alone never retires UDP admission."""
        require(self.direct_udp and player in self.tokens, "Invalid direct departure")
        if self.phase not in ("running", "reviewing", "postgame"):
            await self.close("A player left before the match started.")
            return
        if self.phase == "running" and self.host == self.index(player):
            # No simulation successor may be elected without its validated
            # complete world. This remains the explicit live migration gate.
            await self.close("The host left before a surviving authority was ready.")
            return
        self.peers.pop(player, None)
        self.retire_capability(player)
        if not self.tokens:
            await self.close("Match completed.")
            return
        if self.phase == "running":
            # Native leave/timeout and simulation/voice remain direct UDP.
            # The original roster and result-consent requirement remain fixed.
            return
        if self.host == self.index(player):
            self.host = min(self.index(p) for p in self.tokens)
        if self.phase == "reviewing":
            self.finish_review()
        self.broadcast(self.voice_membership())

    async def depart(self, player):
        if player not in self.peers:
            return
        self.peers.pop(player)
        if self.closed:
            return
        if not self.supports_continuity:
            await self.close()
            return
        self.retire_capability(player)
        if not self.peers:
            await self.close("No players remain.")
        elif self.phase in ("running", "migrating", "awaiting_world", "rebinding"):
            self.continuity.disconnect(self.index(player))
            await self.synchronize_continuity()
        elif self.phase in ("reviewing", "postgame"):
            if self.host == self.index(player):
                self.host = min(self.index(p) for p in self.peers)
            if self.phase == "reviewing":
                try:
                    self.finish_review()
                except sqlite3.Error:
                    logging.getLogger("halo2.match").error("Unable to save surviving peers' confirmed result")
                    await self.close("Unable to save the completed match. Search again.")
                    return
            self.broadcast(self.voice_membership())
        else:
            # Loading has no complete running world to preserve.
            await self.close("A player left before the match started.")

    def finish_review(self):
        if self.phase != "reviewing" or self.confirmations != self.admitted_owners:
            return
        self.api.results.commit(self)
        self.phase, self.deadline = "postgame", time.monotonic() + 900
        for ticket in self.proposal.tickets:
            if ticket in self.api.core.tickets:
                self.api.core.cancel(ticket)
        self.broadcast(json.dumps({"completed": self.result_digest}))
        self.broadcast(self.voice_membership())

    async def world_control(self, player, data):
        actions = {"world_seal", "world_ack", "migration_accept", "migration_rebind"}
        if not actions.intersection(data):
            return False
        require(self.supports_continuity and self.continuity and len(data) == 1,
                "Complete-world continuation is unavailable")
        action, value = next(iter(data.items()))
        require(isinstance(value, dict) and set(value) == {"epoch", "tick", "digest"}
                and type(value["epoch"]) is int and 0 <= value["epoch"] < 2**32
                and type(value["tick"]) is int and 0 <= value["tick"] < 2**64
                and isinstance(value["digest"], str) and len(value["digest"]) == 64
                and all(c in "0123456789abcdef" for c in value["digest"]),
                "Invalid complete-world continuation control")
        if self.phase in ("reviewing", "postgame") and action in ("world_seal", "world_ack"):
            return True  # A late completed-tick proof cannot reopen a finished game.
        if value["epoch"] < self.continuity.epoch and not (
                action == "world_ack" and self.continuity.phase == "awaiting_world"
                and value["epoch"] + 1 == self.continuity.epoch):
            return True  # Discard in-flight old-generation controls without evicting their sender.
        peer = self.index(player)
        before = self.continuity.control()
        arguments = peer, value["epoch"], value["tick"], value["digest"]
        if action == "world_seal":
            self.continuity.seal(*arguments)
            self.broadcast(json.dumps(data))
        elif action == "world_ack":
            self.continuity.acknowledge(*arguments)
        elif action == "migration_accept":
            self.continuity.accept(*arguments)
        else:
            self.continuity.rebind(*arguments)
        if before != self.continuity.control():
            await self.synchronize_continuity()
        return True

    def voice_membership(self):
        return json.dumps({"voice_host": self.host, "voice_members": sum(1 << i for i, (p, _, _) in enumerate(self.players)
                          if self.owners[p] in (self.tokens if self.direct_udp else self.peers))})

    def voice_block_mask(self, player):
        blocks = self.api.directory.voice_blocks
        local = [self.owners[p] if p in self.guests else p for p, _, _ in self.players if self.owners[p] == player]
        return sum(1 << i for i, (other, _, _) in enumerate(self.players)
                   if self.owners[other] != player and any(not blocks.allowed(p,
                      self.owners[other] if other in self.guests else other) for p in local))

    def voice_allowed(self, speaker, listener):
        # One shared playback device must respect every authenticated local
        # user's blocks; guests inherit their connection owner's block policy.
        return not (self.voice_block_mask(self.owners[self.players[listener][0]]) & (1 << speaker))

    async def close(self, reason="A player disconnected. Search again."):
        if self.closed:
            return
        self.closed = True
        for ticket in self.proposal.tickets:
            if ticket in self.api.core.tickets:
                self.api.core.cancel(ticket)
        await asyncio.gather(*(p.socket.close(code=1000, message=reason.encode()[:120])
                               for p in tuple(self.peers.values())), return_exceptions=True)

    def measured_hosts(self):
        """Rank physical owners using reciprocal measured direct paths.

        All-legacy rooms retain their RTT policy until delivered clients are
        upgraded. Once a room supplies upload measurements, missing columns
        cannot silently downgrade selection to an unmeasured host.
        """
        if not any("upload_bytes_per_second" in row
                   for rows in self.direct_ready.values() for row in rows.values()):
            return None
        ranked = {}
        for owner in self.tokens:
            paths = []
            for other in self.tokens:
                if other == owner:
                    continue
                a = self.direct_ready[owner][self.index(other)]
                b = self.direct_ready[other][self.index(owner)]
                if any(row.get("rtt_ms") is None or row.get("upload_bytes_per_second") is None
                       or row.get("loss_per_mille") is None for row in (a, b)):
                    break
                paths.append((max(a["rtt_ms"], b["rtt_ms"]), a["upload_bytes_per_second"],
                              max(a["loss_per_mille"], b["loss_per_mille"])))
            else:
                ranked[owner] = (max((p[2] for p in paths), default=0),
                                 -min((p[1] for p in paths), default=64 * 1024 * 1024),
                                 sum(p[0] for p in paths) // max(1, len(paths)), self.index(owner))
        return ranked

    async def advance(self):
        if self.closed:
            return
        if self.phase == "connecting" and all(len(self.samples.get(p, ())) >= 3 for p in self.tokens) and (
                not self.direct_udp or set(self.direct_ready) == set(self.tokens)):
            # Explicit native relay score, not an assertion about Xbox QoS.
            for a, _, _ in self.players:
                for b, _, _ in self.players:
                    if a < b:
                        if self.owners[a] == self.owners[b]:
                            ms = 0
                        elif self.direct_udp:
                            # DTLS-connected endpoints report direct RTT.
                            # This is native peer QoS, not server-observed Xbox
                            # XNet QoS or anti-cheat proof. Use the worse end.
                            pair = (self.direct_ready[self.owners[a]][self.index(self.owners[b])]["rtt_ms"],
                                    self.direct_ready[self.owners[b]][self.index(self.owners[a])]["rtt_ms"])
                            if None in pair:
                                self.api.core.connections.pop((a, b), None)
                                self.api.core.connections.pop((b, a), None)
                                continue
                            ms = max(pair)
                        else:
                            ms = statistics.median(self.samples[self.owners[a]]) + statistics.median(self.samples[self.owners[b]])
                        score = max(0, 100 - int(ms / 5))
                        self.api.core.connection(a, b, score)
                        self.api.core.connection(b, a, score)
            host_quality = self.measured_hosts() if self.direct_udp else None
            proposal = self.api.core.refresh_connections(self.id, optimize_host=self.direct_udp, host_quality=host_quality)
            if proposal.host is None:
                await self.close("Connection quality is insufficient for this playlist.")
                return
            self.host = self.index(self.owners[proposal.host])
            for player, _, _ in self.players:
                self.api.core.acknowledge(self.id, player, optimize_host=self.direct_udp, host_quality=host_quality)
            self.phase, self.deadline = "loading", time.monotonic() + 180
            for peer in self.peers.values():
                peer.queue.put_nowait(self.control())
        if self.phase == "loading" and len(self.loaded) == len(self.tokens):
            if len(set(self.loaded.values())) != 1:
                await self.close("Map, game data, or native protocol differs between players.")
                return
            self.phase, self.deadline = "running", time.monotonic() + 7200
            if self.supports_continuity:
                self.continuity = PlayerHostedContinuity(
                    [self.index(self.owners[p]) for p, _, _ in self.players], self.host)
                for player, samples in self.samples.items():
                    self.continuity.observe_quality(self.index(player), statistics.median(samples))
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
        async def failed_reader():
            if room.supports_continuity or room.direct_udp:
                await peer.socket.close(code=1008, message=b"Match connection lost")
            else:
                await room.close()
        while not room.closed:
            if room.continuity and room.continuity.deadline is not None and time.monotonic() >= room.continuity.deadline:
                await room.synchronize_continuity()
                if room.closed:
                    return
            mask = room.voice_block_mask(player)
            if mask != blocked:
                try:
                    await asyncio.wait_for(peer.socket.send_str(json.dumps({"voice_blocked": mask})), .5)
                except (ConnectionError, asyncio.TimeoutError):
                    await failed_reader()
                    return
                blocked = mask
            if time.monotonic() > room.deadline or (room.phase == "connecting" and room.id not in self.api.core.proposals):
                await room.close("Match connection or loading timed out. Search again.")
                return
            if not peer.challenge and (time.monotonic() - peer.sent > (.2 if room.phase == "connecting" else 5)):
                peer.challenge, peer.sent = secrets.token_hex(8), time.monotonic()
                await peer.socket.send_str(json.dumps({"ping": peer.challenge}))
            elif peer.challenge and time.monotonic() - peer.sent > 15:
                await failed_reader()
                return
            try:
                packet = await asyncio.wait_for(peer.queue.get(), .05)
            except asyncio.TimeoutError:
                continue
            try:
                if isinstance(packet, str):
                    await asyncio.wait_for(peer.socket.send_str(packet), .5)
                else:
                    offset = 5 if room.supports_continuity else 1
                    if room.supports_continuity and (room.continuity is None or
                            int.from_bytes(packet[1:5], "little") != room.continuity.epoch):
                        continue
                    # The host must receive client microphones for onward
                    # routing, even when its own playback is blocked. Native
                    # host playback obeys the separate server-owned mask.
                    if (room.index(player) != room.host and len(packet) >= offset + 9
                            and packet[offset+6:offset+8] == b"\x06\x00"
                            and not room.voice_allowed(packet[offset+8], room.index(player))):
                        continue
                    await asyncio.wait_for(peer.socket.send_bytes(packet), .5)
            except (ConnectionError, asyncio.TimeoutError):
                await failed_reader()
                return

    async def connect(self, request):
        room = self.rooms.get(request.match_info["room"])
        require(room is not None and not room.closed, "Match no longer exists", 404)
        require(room.direct_udp == request.path.endswith("/signal"), "Wrong match transport", 409)
        bearer = request.headers.get("Authorization", "")
        require(bearer.startswith("Bearer ") and len(bearer) < 128, "Match authorization required", 401)
        player = next((p for p, token in room.tokens.items() if secrets.compare_digest(token, bearer[7:])), None)
        require(player is not None, "Match authorization required", 401)
        require(player not in room.peers or (room.direct_udp and room.phase in ("running", "reviewing", "postgame")),
                "Already connected to match", 409)
        # aiohttp's parser rejects messages whose size equals this ceiling.
        # A complete 1400-byte native datagram plus its route byte is valid.
        socket = web.WebSocketResponse(max_msg_size=120000, compress=False)
        await socket.prepare(request)
        if room.closed or not secrets.compare_digest(room.tokens.get(player, ""), bearer[7:]) or (
                player in room.peers and not room.direct_udp):
            await socket.close(code=1008)
            return socket
        peer = Peer(socket)
        previous = room.peers.get(player)
        room.peers[player] = peer
        if previous:
            await previous.socket.close(code=1001, message=b"Master connection replaced")
        peer.queue.put_nowait(room.voice_auth(player))
        if room.direct_udp and room.phase in ("reviewing", "postgame"):
            peer.queue.put_nowait(json.dumps({"postgame": room.result.hex()}))
        else:
            peer.queue.put_nowait(room.control())
        for packet in room.pending_signals.pop(player, ()):
            peer.queue.put_nowait(packet)
        writer = asyncio.create_task(self.writer(room, player, peer))
        try:
            async for message in socket:
                if room.peers.get(player) is not peer:
                    break
                now = time.monotonic()
                if message.type == WSMsgType.TEXT:
                    peer.budget.take()
                    require(len(message.data) <= 119000, "Invalid match control")
                    data = json.loads(message.data)
                    require(isinstance(data, dict), "Invalid match control")
                    if set(data) == {"pong"}:
                        require(peer.challenge and data["pong"] == peer.challenge, "Invalid connection challenge")
                        room.samples.setdefault(player, []).append((now - peer.sent) * 1000)
                        room.samples[player] = room.samples[player][-3:]
                        peer.challenge = ""
                    elif set(data) == {"leave"}:
                        require(room.direct_udp and data["leave"] is True, "Invalid match departure")
                        if room.peers.get(player) is not peer:
                            break  # A replaced reader cannot retire the newer connection.
                        await room.depart_direct(player)
                        break
                    elif set(data) == {"peer_signal"}:
                        require(room.direct_udp, "Peer signaling is unavailable")
                        signal = data["peer_signal"]
                        require(isinstance(signal, dict) and set(signal) == {"peer", "type", "payload", "mid"}, "Invalid peer signal")
                        target, source = signal["peer"], room.index(player)
                        require(type(target) is int and 0 <= target < len(room.players) and target != source
                                and room.index(room.owners[room.players[target][0]]) == target, "Invalid peer signal destination")
                        require(signal["type"] in (("eos",) if room.eos else ("offer", "answer", "candidate")) and isinstance(signal["payload"], str)
                                and 0 < len(signal["payload"]) <= 16384 and isinstance(signal["mid"], str)
                                and len(signal["mid"]) <= 32, "Invalid peer signal")
                        destination = room.owners[room.players[target][0]]
                        if destination not in room.tokens:
                            # The descriptor is immutable and ICE callbacks
                            # can already be queued when this owner leaves.
                            # Retired endpoints cannot consume signaling or
                            # terminate the surviving running/reviewing room.
                            continue
                        key = source, target
                        room.signal_counts[key] = room.signal_counts.get(key, 0) + 1
                        require(room.signal_counts[key] <= 65, "Too many peer candidates")
                        if room.eos:
                            user = signal["payload"]
                            require(len(user) == 32 and all(c in "0123456789abcdef" for c in user) and not signal["mid"], "Invalid EOS identity")
                            require(room.eos_users.get(source, user) == user and all(slot == source or value != user
                                    for slot, value in room.eos_users.items()), "EOS identity changed or reused")
                            room.eos_users[source] = user
                        elif signal["type"] != "candidate":
                            require(room.phase in ("connecting", "loading") and not signal["mid"] and key not in room.signaled_descriptions and
                                    signal["type"] == ("offer" if source < target else "answer"), "Invalid peer description")
                            room.signaled_descriptions.add(key)
                        else:
                            require(len(signal["payload"]) <= 1024, "Invalid peer candidate")
                        packet = json.dumps({"peer_signal": dict(signal, peer=source)})
                        if destination in room.peers:
                            require(not room.peers[destination].queue.full(), "Peer signaling reader stalled")
                            room.peers[destination].queue.put_nowait(packet)
                        else:
                            pending = room.pending_signals.setdefault(destination, [])
                            require(len(pending) < 512, "Too many pending peer candidates")
                            pending.append(packet)
                    elif set(data) == {"peer_ready"}:
                        require(room.direct_udp and room.phase == "connecting" and player not in room.direct_ready,
                                "Invalid direct connection admission")
                        rows = data["peer_ready"]
                        expected = {room.index(p) for p in room.tokens if p != player}
                        require(isinstance(rows, list) and len(rows) == len(expected), "Missing direct peer connections")
                        quality = {}
                        for row in rows:
                            require(isinstance(row, dict) and set(row) in ({"peer", "rtt_ms"},
                                    {"peer", "rtt_ms", "upload_bytes_per_second", "loss_per_mille"}) and type(row["peer"]) is int
                                    and row["peer"] in expected and row["peer"] not in quality and
                                    (row["rtt_ms"] is None or (type(row["rtt_ms"]) is int and 0 <= row["rtt_ms"] <= 60000)), "Invalid direct peer quality")
                            if "upload_bytes_per_second" in row:
                                upload, loss = row["upload_bytes_per_second"], row["loss_per_mille"]
                                require((row["rtt_ms"] is None and upload is None and loss is None) or
                                        (row["rtt_ms"] is not None and type(upload) is int and 0 < upload <= 64 * 1024 * 1024
                                         and type(loss) is int and 0 <= loss <= 1000), "Invalid direct upload measurement")
                            quality[row["peer"]] = dict(row)
                        room.direct_ready[player] = quality
                    elif set(data) == {"loaded"}:
                        require(room.phase == "loading" and isinstance(data["loaded"], str)
                                and len(data["loaded"]) == 24 and all(c in "0123456789abcdef:" for c in data["loaded"]),
                                "Invalid game-data admission")
                        if room.supports_continuity or room.direct_udp:
                            value = data["loaded"]
                            require(value[16] == ":" and all(c in "0123456789abcdef" for c in value[:16] + value[17:])
                                    and int(value[17:], 16) <= 0xffff, "Invalid native continuation protocol admission")
                        room.loaded[player] = data["loaded"]
                    elif await room.world_control(player, data):
                        pass
                    elif set(data) == {"postgame"}:
                        report = data["postgame"]
                        require(room.index(player) == room.host and
                                (room.phase == "running" or (room.direct_udp and room.phase in ("reviewing", "postgame")))
                                and (room.direct_udp or len(room.peers) == len(room.tokens)), "Invalid postgame report")
                        if room.direct_udp and room.phase in ("reviewing", "postgame"):
                            require(report == room.result.hex(), "Postgame retry differs from the original report")
                            peer.queue.put_nowait(json.dumps({"postgame": report}))
                            continue
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
                        if room.phase == "postgame":
                            if room.direct_udp:
                                peer.queue.put_nowait(json.dumps({"completed": room.result_digest}))
                                peer.queue.put_nowait(room.voice_membership())
                            continue
                        room.finish_review()
                    else:
                        require(False, "Unknown match control")
                    await room.advance()
                elif message.type == WSMsgType.BINARY:
                    # v12 has no payload relay or silent TCP fallback. The
                    # master owns signaling, membership and results only.
                    require(not room.direct_udp, "Gameplay and voice require a direct player connection")
                    packet = message.data
                    source = room.index(player)
                    offset = 5 if room.supports_continuity else 1
                    require(offset + 7 <= len(packet) <= offset + 1400, "Invalid game packet")
                    if room.supports_continuity:
                        epoch = int.from_bytes(packet[1:5], "little")
                        if room.continuity is None or source not in room.continuity.active or epoch != room.continuity.epoch or room.phase in ("migrating", "awaiting_world", "rebinding"):
                            peer.budget.take()
                            continue  # In-flight old-generation packets cannot end the new room.
                    require(room.phase in ("running", "reviewing", "postgame"), "Invalid game packet phase")
                    target, native = packet[0], packet[offset:]
                    require(native[:4] == b"H2N2", "Invalid native packet")
                    if room.supports_continuity:
                        require(int.from_bytes(native[4:6], "little") == int(room.loaded[player].split(":")[1], 16),
                                "Native packet differs from the admitted gameplay protocol")
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
                            verify_voice(room, epoch if room.supports_continuity else 0, native)
                            require(native[8] < len(room.players)
                                    and (source == room.host or room.owners[room.players[native[8]][0]] == player), "Invalid voice identity")
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
                                and not room.voice_allowed(speaker, target)):
                            continue
                    else:
                        peer.budget.take(game=target)
                    destination = room.peers.get(room.owners[room.players[target][0]])
                    if destination is None and (room.phase == "postgame" or room.supports_continuity):
                        continue
                    if destination is not None and destination.queue.full() and room.supports_continuity:
                        await destination.socket.close(code=1008, message=b"Match reader stalled")
                        continue
                    require(destination is not None and not destination.queue.full(), "Match reader stalled", 503)
                    prefix = bytes([source]) + (epoch.to_bytes(4, "little") if room.supports_continuity else b"")
                    destination.queue.put_nowait(prefix + native)
                else:
                    break
        except (Problem, web.HTTPException, ValueError, KeyError, TypeError, asyncio.QueueFull) as error:
            reason = error.message if isinstance(error, Problem) else "Invalid match control"
            logging.getLogger("halo2.match").warning("Match peer rejected: %s", reason)
            if room.direct_udp and room.phase in ("running", "reviewing", "postgame"):
                if room.peers.get(player) is peer:
                    await room.depart_direct(player)
                await socket.close(code=1008, message=reason.encode()[:120])
            elif room.supports_continuity:
                await socket.close(code=1008, message=reason.encode()[:120])
            else:
                await room.close(reason)
        except sqlite3.Error:
            logging.getLogger("halo2.match").error("Unable to save confirmed match result")
            await room.close("Unable to save the completed match. Search again.")
        finally:
            writer.cancel()
            with contextlib.suppress(asyncio.CancelledError):
                await writer
            if room.direct_udp and room.phase in ("running", "reviewing", "postgame"):
                # Losing a master TCP socket does not mean losing the player's
                # UDP connection or world. Keep admission for result retries.
                if room.peers.get(player) is peer:
                    room.peers.pop(player, None)
            elif room.supports_continuity:
                await room.depart(player)
            elif room.phase == "postgame":
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
                web.get("/v1/matches/{room}/signal", self.connect),
                web.get("/v1/matches/{room}/relay", self.connect)]
