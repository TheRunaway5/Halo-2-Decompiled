"""Authenticated native match setup and legacy relay, not Xbox wire protocol.

Direct rooms exchange gameplay/media only between players. The coordinator
receives admitted peer measurement summaries for setup, never probe payloads.
These authenticated reports are client claims, not anti-cheat attestations.
"""
import asyncio
import contextlib
import json
import logging
import sqlite3
import time
import weakref
from dataclasses import dataclass
from types import MappingProxyType
from aiohttp import web, WSMsgType
from .protocol import require, Problem, fields
from .couch_players import NATIVE_COUCH, NATIVE_CONTINUITY, NATIVE_DIRECT, NATIVE_EOS, NATIVE_EOS_ACCOUNT, NATIVE_ACCURACY, NATIVE_OPTIONS
from .match_continuity import PlayerHostedContinuity
from .native_matchmaking import native, _response


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
        self.eos_account = self.compatibility in (NATIVE_EOS_ACCOUNT, *NATIVE_OPTIONS)
        self.eos = self.compatibility in (NATIVE_EOS, NATIVE_EOS_ACCOUNT, *NATIVE_OPTIONS)
        self.direct_udp = self.compatibility in (NATIVE_DIRECT, NATIVE_EOS, NATIVE_EOS_ACCOUNT, *NATIVE_OPTIONS)
        # Only this client cohort compares native shot/hit counters against
        # its accepted terminal snapshot before confirming the report.
        self.supports_accuracy = self.compatibility == NATIVE_ACCURACY
        self.players = [(p, names[p], team) for team, players in enumerate(proposal.teams) for p in players]
        self.admitted_owners = frozenset(self.owners.values())
        self._continuity = None
        self._room_final = None
        self.lifecycle_clock = time.monotonic
        # Native per-room presentation identities; never Discord snowflakes or
        # persistent account IDs. These are not retail Xbox Live XUIDs.
        self.ranked = api.core.playlists[proposal.playlist].ranked
        self.team_mode = bool(api.core.playlists[proposal.playlist].team_size)
        configuration = api.core.entry(proposal.playlist).rank_configuration
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
        self.peers = {}
        # TU1.5 start-match owner 0x76360, normal timer path 0x76481 (retail
        # locator 0x72700): one session-owned 10000 ms countdown. The service
        # team-diversity deferral follows 0x721B0. Original Live policy bit31
        # has no replacement-auth equivalent and is never synthesized here.
        # Integer monotonic milliseconds never require synchronized PCs.
        self.clock = lambda: int(api.clock() * 1000)
        self.created_wall = api.directory.store.now()
        self._result_final = None
        result = api.results.open(self)
        self.identities = MappingProxyType(result['identities'])
        self.admitted_owners = frozenset(result['admitted_owners'])
        self.supports_accuracy = result['accuracy']
        api.core._call('api_room_admit', id=self.id)
        self.room_call('room_state_get')

    @property
    def continuity(self):
        return self._continuity

    @property
    def result(self):
        return self.api.results.report(self)

    @property
    def result_digest(self):
        return self.api.results.snapshot(self)['digest']

    @property
    def confirmations(self):
        return frozenset(self.api.results.snapshot(self)['confirmations'])

    def descriptor(self, player):
        return self.room_call('room_wire_descriptor', player=player,
                              stun_servers=list(self.api.directory.settings.peer_stun_urls))

    def wire_snapshot(self):
        if self.closed:
            return dict(tokens={}, eos_users={}, eos_verified={}, pending_signals={})
        return self.room_call('room_wire_snapshot')

    @property
    def tokens(self): return MappingProxyType(self.wire_snapshot()['tokens'])
    @property
    def eos_users(self): return MappingProxyType({int(k):v for k,v in self.wire_snapshot()['eos_users'].items()})
    @property
    def eos_verified(self): return MappingProxyType(self.wire_snapshot()['eos_verified'])
    @property
    def pending_signals(self):
        return MappingProxyType(self.wire_snapshot()['pending_signals'])

    def index(self, player):
        return next(i for i, row in enumerate(self.players) if row[0] == player)

    def active_member(self, player):
        return not self.closed and self.room_call('room_state_member', player=player)

    def setup_call(self, op, **values):
        return self.api.core._call(op, id=self.id, active=tuple(self.tokens),
            connected=tuple(self.peers), phase=self.phase, closed=self.closed,
            clock_ms=self.clock(), io_now=self.lifecycle_clock(), **values)

    @property
    def start_clock(self):
        return self.api.core._call('room_setup_clock', id=self.id)

    @property
    def samples(self):
        rows = self.api.core._call('room_setup_snapshot', id=self.id)['samples']
        return MappingProxyType({who: tuple(values) for who, values in rows.items()})

    @property
    def direct_ready(self):
        rows = self.api.core._call('room_setup_snapshot', id=self.id)['direct_ready']
        return MappingProxyType({who: MappingProxyType({int(peer): MappingProxyType(row)
            for peer, row in reports.items()}) for who, reports in rows.items()})

    @property
    def loaded(self):
        return MappingProxyType(self.api.core._call('room_setup_snapshot', id=self.id)['loaded'])

    def start_countdown_ms(self):
        return self.setup_call('room_setup_start')

    def start_ready(self):
        return self.setup_call('room_setup_ready')

    def record_failure(self, kind="search_failed", owner=None):
        self.api.core._call('api_room_failure', id=self.id, phase=self.phase, kind=kind, player=owner)

    def voice_auth(self, player):
        peer = self.peers[player]
        return json.dumps(self.room_call('room_packets_voice_auth', player=player,
                                        connection=peer.connection))

    def privacy_facts(self, pairs):
        # Account lookup IO only; Rust selects the admitted subjects and
        # decides guest inheritance, device unions and each route's mask.
        blocks = self.api.directory.voice_blocks
        return [dict(a=a, b=b, allowed=blocks.allowed(a,b)) for a,b in pairs]

    def packet_call(self, op, player, peer, **values):
        request = dict(player=player, connection=peer.connection,
                       packet_now=peer.packet_clock(),
                       full=[who for who,p in self.peers.items() if p.queue.full()], **values)
        effect = self.room_call(op, **request)
        if effect['kind'] == 'privacy':
            effect = self.room_call(op, facts=self.privacy_facts(effect['pairs']), **request)
        return effect

    def control(self):
        return json.dumps(self.room_call('room_state_control'))

    def connected_seats(self, player, peer):
        return self.room_call('room_control_presence', player=player,
                              connection=peer.connection, control_now=peer.control_clock())

    def control_call(self, op, player, peer, **values):
        request = dict(player=player, connection=peer.connection,
            control_now=peer.control_clock(), packet_now=peer.packet_clock(),
            clock_ms=self.clock(), full=[who for who,p in self.peers.items() if p.queue.full()], **values)
        effect = self.room_call(op, **request)
        if effect['kind'] == 'privacy':
            effect = self.room_call(op, facts=self.privacy_facts(effect['pairs']), **request)
        return effect

    def _prepare_controls(self, effect):
        if effect['continuity']:
            self._continuity = PlayerHostedContinuity.attach(self.api.core, self.id)
        for item in effect['remove']:
            peer = self.peers.get(item['player'])
            if peer is not None and peer.connection == item['connection']:
                self.peers.pop(item['player'])
                if item['close']:
                    asyncio.create_task(peer.socket.close(code=item['code'], message=item['reason'].encode()[:120]))
        if effect['commit']:
            self.api.results.commit(self)
            completed = self.room_call('room_control_review', full=[who for who,p in self.peers.items() if p.queue.full()])
            require(not completed['commit'], 'Native result completion did not consume its transaction', 503)
            for key in ('publications','remove','disconnect'):
                effect[key].extend(completed[key])
            if completed['close'] is not None:
                effect['close'] = completed['close']
            effect['stop'] = effect['stop'] or completed['stop']
        for item in effect['disconnect']:
            peer = self.peers.get(item['player'])
            if peer is not None and peer.connection == item['connection']:
                asyncio.create_task(peer.socket.close(code=item['code'], message=item['reason'].encode()[:120]))
        for item in effect['publications']:
            if item['immediate']:
                continue
            peer = self.peers.get(item['player'])
            if peer is not None and peer.connection == item['connection']:
                try:
                    peer.queue.put_nowait(json.dumps(item['packet']))
                except asyncio.QueueFull:
                    failure = self.control_call('room_control_stalled', item['player'], peer)
                    plan = failure['close']
                    asyncio.create_task(peer.socket.close(code=plan['code'], message=plan['reason'].encode()[:120]))
        return effect

    async def apply_controls(self, effect, source=None):
        effect = self._prepare_controls(effect)
        for item in effect['publications']:
            if not item['immediate']:
                continue
            peer = self.peers.get(item['player'])
            if peer is not None and peer.connection == item['connection']:
                try:
                    await asyncio.wait_for(peer.socket.send_str(json.dumps(item['packet'])), .5)
                except (ConnectionError, asyncio.TimeoutError):
                    if self.closed:
                        return True
                    failure = self.control_call('room_control_io_failure', item['player'], peer)
                    await self.apply_controls(failure, peer)
                    return True
        plan = effect['close']
        if plan is not None:
            if plan['scope'] == 'room':
                await self.close(plan['reason'])
            elif source is not None:
                await source.socket.close(code=plan['code'], message=plan['reason'].encode()[:120])
        return effect['stop'] or self.closed

    def room_call(self, op, **values):
        return self.api.core._call(op, id=self.id, io_now=self.lifecycle_clock(),
            peers=[dict(player=who, native_handoff=p.native_handoff,
                        departure_game_over=p.departure_game_over) for who, p in self.peers.items()], **values)

    def room_state(self):
        return self._room_final if self._room_final is not None else self.api.core._call(
            'room_state_get', id=self.id, io_now=self.lifecycle_clock())

    def expired(self):
        return not self.closed and self.room_call('room_state_expired')

    async def synchronize_continuity(self):
        if not self.closed:
            await self.apply_controls(self.room_call('room_control_synchronize'))

    def retire_capability(self, player):
        self.room_call('room_state_retire', player=player)

    async def depart_direct(self, player):
        await self._depart(player, True)

    async def depart(self, player):
        if player in self.peers and not self.closed:
            await self._depart(player, False)

    async def _depart(self, player, direct):
        peer = self.peers.get(player)
        effect = self.room_call('room_control_depart', player=player, direct=direct,
                                connection=peer.connection if peer else None)
        await self.apply_controls(effect, peer)

    async def socket_lost(self, player):
        if self.closed:
            self.peers.pop(player, None)
            return
        peer = self.peers.get(player)
        effect = self.room_call('room_control_socket_lost', player=player,
                                connection=peer.connection if peer else None)
        await self.apply_controls(effect, peer)

    def finish_review(self):
        effect = self._prepare_controls(self.room_call('room_control_review'))
        require(effect['close'] is None and all(not p['immediate'] for p in effect['publications']),
                'Unexpected native result IO continuation', 503)

    def completed(self, peer):
        return json.dumps(self.room_call('room_control_completed', player=peer.player, connection=peer.connection))

    async def world_control(self, player, data):
        effect = self.room_call('room_control_world', player=player, data=data)
        await self.apply_controls(effect, self.peers.get(player))
        return effect['handled']

    def voice_membership(self):
        return json.dumps(self.room_call('room_control_voice'))

    def voice_block_mask(self, player, peer=None):
        peer = self.peers[player] if peer is None else peer
        request = dict(player=player, connection=peer.connection)
        pairs = self.room_call('room_packets_privacy_plan', **request)
        return self.room_call('room_packets_mask', facts=self.privacy_facts(pairs), **request)

    async def close(self, reason="A player disconnected. Search again."):
        if self.closed:
            return
        self._room_final = MappingProxyType(self.room_call('room_state_close'))
        self.api.results.retire(self)
        self.api.core._call('api_room_close', id=self.id, phase=self.phase)
        self.api.core._call('room_setup_close', id=self.id)
        await asyncio.gather(*(p.socket.close(code=1000, message=reason.encode()[:120])
                               for p in tuple(self.peers.values())), return_exceptions=True)

    def measured_hosts(self):
        rows = self.api.core._call('room_setup_measured_hosts', id=self.id)
        return None if rows is None else {who: tuple(key) for who, key in rows.items()}

    async def advance(self):
        if not self.closed:
            await self.apply_controls(self.room_call('room_control_advance', clock_ms=self.clock()))


for _room_field in ('phase', 'host', 'closed', 'created', 'deadline', 'departed_slots',
                    'result_recorded', 'setup_host_departed'):
    setattr(Room, _room_field, property(lambda self, name=_room_field: self.room_state()[name]))


class PacketBudget:
    """Standalone Rust budget fixture; live connections own their own budgets."""
    def __init__(self, *, clock=time.monotonic):
        self.clock = clock
        lib = native()
        self._handle = _response(lib.h2_matchmaking_object_open, request={'packet_budget':{}})
        self._finalizer = weakref.finalize(self, lib.h2_matchmaking_owner_drop, self._handle)

    def take(self, audio=None, *, game=None):
        return _response(native().h2_matchmaking_owner_request, self._handle,
            request=dict(op='take', now=self.clock(), audio=audio, game=game))

    def close(self):
        self._finalizer()
        self._handle = 0


@dataclass(frozen=True)
class Outbound:
    packet: bytes
    target: int
    origin_connection: int
    recipient_connection: int


class Peer:
    def __init__(self, socket, admission):
        self.socket, self.queue = socket, asyncio.Queue(maxsize=512)
        self.control_clock = time.monotonic
        self.packet_clock = time.monotonic
        self._connection = admission['connection']
        self._player = admission['player']
        self._features = MappingProxyType(admission['features'])

    @property
    def player(self): return self._player
    @property
    def connection(self): return self._connection
    @property
    def completion_detail(self): return self._features['completion_detail']
    @property
    def native_handoff(self): return self._features['native_handoff']
    @property
    def departure_game_over(self): return self._features['departure_game_over']


class MatchTransport:
    def __init__(self, api):
        self.api, self.rooms = api, {}
        api.transport = self

    async def cleanup(self):
        for room in tuple(self.rooms.values()):
            if room.expired():
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

    def eos_owner(self, request, previous=None):
        room = self.rooms.get(request.match_info['room'])
        require(room is not None and not room.closed and (previous is None or room is previous[0]),
                'Match no longer exists', 404)
        admission = room.room_call('room_wire_authorize', route='eos',
            authorization=request.headers.get('Authorization',''), prior=None if previous is None else previous[2])
        return room, admission['player'], admission

    async def eos_token(self, request):
        owner = self.eos_owner(request)
        body = await self.api.directory.body(request)
        fields(body, ())
        room, player, admission = self.eos_owner(request, owner)
        self.api.directory.rate("eos-room:" + player, 120)
        name = admission['name']
        token = self.api.directory.eos_identity.token(player, name)
        return web.json_response(dict(token=token, expires_in=120))

    async def eos_proof(self, request):
        owner = self.eos_owner(request)
        body = await self.api.directory.body(request)
        fields(body, ("token",))
        owner = self.eos_owner(request, owner)
        self.api.directory.rate("eos-room:" + owner[1], 120)
        identity = self.api.directory.eos_identity
        puid = identity.proof_subject(body["token"])
        account = await identity.verify(puid, body["token"], principal=owner[1])
        room, player, admission = self.eos_owner(request, owner)
        proof = dict(route='eos', authorization=request.headers.get('Authorization',''),
                     prior=admission, account=account, puid=puid)
        room.room_call('room_wire_proof_validate', **proof)
        # No await separates capability revalidation, immutable binding and the
        # room-local proof receipt. Social/account sessions are left untouched.
        identity.bind_player(player, puid)
        room.room_call('room_wire_proof_commit', **proof)
        return web.json_response(dict(eos_product_user_id=puid))

    async def writer(self, room, player, peer):
        while not room.closed:
            effect = room.control_call('room_control_tick', player, peer)
            if await room.apply_controls(effect, peer):
                return
            try:
                packet = await asyncio.wait_for(peer.queue.get(), .05)
            except asyncio.TimeoutError:
                continue
            try:
                if isinstance(packet, str):
                    await asyncio.wait_for(peer.socket.send_str(packet), .5)
                else:
                    effect = room.packet_call('room_packets_delivery', player, peer,
                        packet=packet.packet.hex(), target=packet.target,
                        origin_connection=packet.origin_connection,
                        recipient_connection=packet.recipient_connection)
                    if effect['kind'] == 'send':
                        await asyncio.wait_for(peer.socket.send_bytes(packet.packet), .5)
            except (ConnectionError, asyncio.TimeoutError, Problem):
                if not room.closed:
                    await room.apply_controls(room.control_call('room_control_io_failure', player, peer), peer)
                return

    async def connect(self, request):
        room = self.rooms.get(request.match_info["room"])
        require(room is not None and not room.closed, "Match no longer exists", 404)
        authorization = request.headers.get('Authorization','')
        route = 'signal' if request.path.endswith('/signal') else 'relay'
        admission = room.room_call('room_wire_authorize', route=route, authorization=authorization, prior=None)
        # aiohttp's parser rejects messages whose size equals this ceiling.
        # A complete 1400-byte native datagram plus its route byte is valid.
        socket = web.WebSocketResponse(max_msg_size=120000, compress=False)
        await socket.prepare(request)
        try:
            attached = room.room_call('room_wire_attach', route=route, authorization=authorization, prior=admission,
                completion=request.query.get('completion'), native_handoff=request.query.get('native_handoff'),
                departure_game_over=request.query.get('departure_game_over'))
        except Problem:
            await socket.close(code=1008)
            return socket
        player = attached['player']
        peer = Peer(socket, attached)
        previous = room.peers.get(player)
        room.peers[player] = peer
        if previous:
            await previous.socket.close(code=1001, message=b"Master connection replaced")
        peer.queue.put_nowait(json.dumps(attached['voice_auth']))
        for packet in attached['publications']:
            peer.queue.put_nowait(json.dumps(packet))
        writer = asyncio.create_task(self.writer(room, player, peer))
        try:
            while True:
                pending = room.room_call('room_wire_drain', player=player, connection=peer.connection)
                if not pending:
                    break
                for packet in pending:
                    await asyncio.wait_for(peer.queue.put(json.dumps(packet)), .5)
            async for message in socket:
                if room.closed or room.peers.get(player) is not peer or not room.room_call('room_wire_current', player=player, connection=peer.connection):
                    break
                if message.type == WSMsgType.TEXT:
                    effect = room.control_call('room_control_receive', player, peer, text=message.data)
                    if await room.apply_controls(effect, peer):
                        break
                elif message.type == WSMsgType.BINARY:
                    effect = room.packet_call('room_packets_route', player, peer,
                                              packet=message.data.hex())
                    if effect['close_peer'] is not None:
                        await room.peers[effect['close_peer']].socket.close(code=1008, message=b"Match reader stalled")
                    if effect['deliver'] is not None:
                        room.peers[effect['deliver']].queue.put_nowait(
                            Outbound(bytes.fromhex(effect['packet']), effect['target'],
                                     effect['origin_connection'], effect['recipient_connection']))
                else:
                    break
        except (Problem, web.HTTPException, ValueError, KeyError, TypeError, asyncio.QueueFull, asyncio.TimeoutError) as error:
            reason = error.message if isinstance(error, Problem) else "Invalid match control"
            logging.getLogger("halo2.match").warning("Match peer rejected: %s", reason)
            if not room.closed:
                effect = room.control_call('room_control_rejected', player, peer, reason=reason)
                await room.apply_controls(effect, peer)
            else:
                await socket.close(code=1008, message=reason.encode()[:120])
        except sqlite3.Error:
            logging.getLogger("halo2.match").error("Unable to save confirmed match result")
            await room.close("Unable to save the completed match. Search again.")
        finally:
            writer.cancel()
            with contextlib.suppress(asyncio.CancelledError):
                await writer
            if room.peers.get(player) is peer:
                if room.closed or room.room_call('room_wire_detach', player=player, connection=peer.connection):
                    await room.socket_lost(player)
                else:
                    room.peers.pop(player, None)
        return socket

    async def close(self, app):
        await asyncio.gather(*(r.close("Match service is restarting.") for r in tuple(self.rooms.values())))

    def routes(self):
        return [web.get("/v1/matchmaking/transport", self.descriptor),
                web.post("/v1/matches/{room}/eos-token", self.eos_token),
                web.post("/v1/matches/{room}/eos-proof", self.eos_proof),
                web.get("/v1/matches/{room}/signal", self.connect),
                web.get("/v1/matches/{room}/relay", self.connect)]
