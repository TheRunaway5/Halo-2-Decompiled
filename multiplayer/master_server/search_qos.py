"""Server-timed probes of the native relay route before gathering selection.

A probe visits two authenticated native sockets in sequence. The server owns
the clock and both unpredictable challenges; clients return only a challenge.
This measures a complete relay-route cycle, not Xbox XNet/direct-peer QoS or
the sum of independently sampled client-to-server RTTs. No player identities,
ratings, addresses or measurements are sent to the probing clients.
"""
import asyncio
import contextlib
from dataclasses import dataclass, field
import secrets
import statistics
import time

from aiohttp import web, WSMsgType
from .protocol import fields, parse, require


@dataclass
class Peer:
    socket: object
    session: object
    key: str
    queue: asyncio.Queue = field(default_factory=lambda: asyncio.Queue(8))
    ping: str = ""
    sent: float = 0
    credits: float = 20
    credit_at: float = 0
    probe_credits: float = 16
    probe_at: float = 0


@dataclass
class Probe:
    pair: tuple[str, str]
    nonce: str
    expires: float
    waiting: str
    started: float | None = None
    second: bool = False


@dataclass(frozen=True)
class Measurement:
    milliseconds: int
    expires: float


@dataclass
class Samples:
    expires: float
    values: list[int] = field(default_factory=list)


class SearchQos:
    def __init__(self, api, *, clock=time.monotonic, lease=30, timeout=4):
        self.api, self.clock, self.lease, self.timeout = api, clock, lease, timeout
        self.peers, self.probes, self.measurements, self.samples = {}, {}, {}, {}
        self.cleanup_at = 0
        api.search_qos = self

    def discard(self, player):
        for table in (self.probes, self.measurements, self.samples):
            for pair in tuple(table):
                if player in pair:
                    del table[pair]

    def cleanup(self):
        now = self.clock()
        if now < self.cleanup_at:
            return
        self.cleanup_at = now+.25
        active = {p.id for t in self.api.core.tickets.values() for p in t.players}
        for table in (self.probes, self.measurements, self.samples):
            for pair, value in tuple(table.items()):
                if value.expires <= now or not set(pair) <= active:
                    del table[pair]
                    self.samples.pop(pair, None)

    def ready(self, player):
        peer = self.peers.get(player)
        return (peer is not None and not peer.socket.closed and
                self.api.sessions.get(peer.key) is peer.session and peer.session.expires > self.api.clock())

    def measurement(self, a, b):
        self.cleanup()
        if not self.ready(a) or not self.ready(b) or self.api.ticket(a) is None or self.api.ticket(b) is None:
            return None
        value = self.measurements.get(tuple(sorted((a, b))))
        return value if value and value.expires > self.clock() else None

    def request(self, a, b):
        """Trusted queue owner asks for a pair; never a client-supplied target."""
        self.cleanup()
        pair = tuple(sorted((a, b)))
        active = {p.id for t in self.api.core.tickets.values() for p in t.players}
        if a == b or not set(pair) <= active or not all(self.ready(p) for p in pair):
            return
        if pair in self.probes or pair in self.measurements:
            return True
        # A busy gathering host cannot allocate unbounded queues or challenges.
        if len(self.measurements) >= 50000 or len(self.probes) >= 1000 or any(sum(p in key for key in self.probes) >= 4 for p in pair):
            return
        peer = self.peers[pair[0]]
        if peer.queue.full():
            return
        now = self.clock()
        # Reserve reply capacity on both legs before issuing a challenge. A
        # busy real host must not trip the inbound flood limit by answering
        # probes that the service itself scheduled.
        participants = [self.peers[p] for p in pair]
        for participant in participants:
            participant.probe_credits = min(16., participant.probe_credits + max(0.,now-participant.probe_at)*32)
            participant.probe_at = now
        if any(p.probe_credits < 1 for p in participants):
            return
        for participant in participants:
            participant.probe_credits -= 1
        probe = Probe(pair, secrets.token_hex(16), self.clock()+self.timeout, pair[0])
        self.probes[pair] = probe
        peer.queue.put_nowait(probe)
        return True

    def pending(self, a, b):
        """Actual live native work; a refused allocation is not a fake handle."""
        self.cleanup()
        pair = tuple(sorted((a,b)))
        return pair in self.probes or pair in self.samples

    def echo(self, player, nonce):
        self.cleanup()
        probe = next((p for p in self.probes.values() if p.waiting == player and p.nonce == nonce), None)
        require(probe is not None and probe.started is not None and probe.expires > self.clock(), "Unexpected route probe", 400)
        if not probe.second:
            other = self.peers.get(probe.pair[1])
            if other is None or not self.ready(probe.pair[1]) or other.queue.full():
                self.probes.pop(probe.pair, None)
                self.samples.pop(probe.pair, None)
                return
            probe.second, probe.waiting, probe.nonce = True, probe.pair[1], secrets.token_hex(16)
            other.queue.put_nowait(probe)
            return
        elapsed = max(0, int((self.clock()-probe.started)*1000))
        self.probes.pop(probe.pair, None)
        samples = self.samples.setdefault(probe.pair, Samples(self.clock()+3*self.timeout))
        samples.values.append(elapsed)
        if len(samples.values) == 3:
            self.measurements[probe.pair] = Measurement(int(statistics.median(samples.values)), self.clock()+self.lease)
            del self.samples[probe.pair]
        else:
            self.request(*probe.pair)

    async def writer(self, player, peer):
        try:
            while self.ready(player) and self.peers.get(player) is peer:
                self.cleanup()
                now = self.clock()
                if not peer.ping and now-peer.sent >= 5:
                    peer.ping, peer.sent = secrets.token_hex(16), now
                    await asyncio.wait_for(peer.socket.send_json({"ping": peer.ping}), .5)
                elif peer.ping and now-peer.sent > 15:
                    break
                try:
                    probe = await asyncio.wait_for(peer.queue.get(), .05)
                except asyncio.TimeoutError:
                    continue
                if self.probes.get(probe.pair) is not probe or probe.waiting != player:
                    continue
                if not probe.second:
                    probe.started = self.clock()
                await asyncio.wait_for(peer.socket.send_json({"probe": probe.nonce}), .5)
        finally:
            await peer.socket.close()

    async def connect(self, request):
        key, session = self.api.session(request)
        require(session.compatibility == "halo2-native-queue-v8", "Update Halo 2 for gathering QoS", 409)
        require(session.player not in self.peers, "Search connection already open", 409)
        socket = web.WebSocketResponse(max_msg_size=256, compress=False, autoping=True)
        await socket.prepare(request)
        # A logout/rotation may run while the upgrade writes to the socket.
        if self.api.sessions.get(key) is not session or session.player in self.peers:
            await socket.close()
            return socket
        peer = Peer(socket, session, key, credit_at=self.clock(), probe_at=self.clock())
        self.peers[session.player] = peer
        writer = asyncio.create_task(self.writer(session.player, peer))
        try:
            async for message in socket:
                if message.type in (WSMsgType.CLOSE, WSMsgType.CLOSED, WSMsgType.ERROR):
                    break
                require(self.ready(session.player), "Search authorization expired", 401)
                now = self.clock()
                peer.credits = min(20., peer.credits + max(0., now-peer.credit_at)*40)
                peer.credit_at = now
                require(peer.credits >= 1, "Search control flood", 429)
                peer.credits -= 1
                require(message.type == WSMsgType.TEXT, "Invalid search control", 400)
                body = parse(message.data.encode())
                if set(body) == {"pong"}:
                    require(peer.ping and body["pong"] == peer.ping, "Unexpected search heartbeat", 400)
                    peer.ping = ""
                else:
                    fields(body, ("echo",))
                    require(type(body["echo"]) is str and len(body["echo"]) == 32, "Invalid route challenge", 400)
                    self.echo(session.player, body["echo"])
        except Exception:
            # No payload, nonce, identity or bearer is copied into logs/errors.
            pass
        finally:
            if self.peers.get(session.player) is peer:
                del self.peers[session.player]
                self.discard(session.player)
            writer.cancel()
            with contextlib.suppress(asyncio.CancelledError, ConnectionError, asyncio.TimeoutError):
                await writer
            await socket.close()
        return socket

    async def close(self, app):
        await asyncio.gather(*(p.socket.close() for p in tuple(self.peers.values())))

    def routes(self):
        return [web.get("/v1/matchmaking/qos", self.connect)]
