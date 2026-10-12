"""Opt-in clan Opus relay. Native extension, separate from game voice routing.

No recording. TLS terminates at the operator's reverse proxy. Each queued frame
is checked against current account sessions and clan membership before delivery.
"""
import asyncio
import contextlib
import time
import uuid
from aiohttp import web, WSMsgType
from .protocol import require


class Peer:
    def __init__(self, key, player, clan, socket):
        self.key, self.player, self.clan, self.socket = key, player, clan, socket
        self.queue = asyncio.Queue(maxsize=5)


class ClanVoice:
    group_name = "Clan"
    def __init__(self, accounts):
        self.accounts = accounts
        self.peers = {}

    def member(self, who):
        return self.accounts.member(who)

    def identity(self, who):
        return who

    def valid(self, peer):
        session = self.accounts.sessions.get(peer.key)
        member = self.member(peer.player)
        return (session is not None and session.expires > self.accounts.clock()
                and session.player == peer.player and member is not None
                and member[0] == peer.clan)

    async def writer(self, peer):
        while not peer.socket.closed:
            if not self.valid(peer):
                await peer.socket.close(code=1008, message=b"Voice authorization ended")
                return
            try:
                source, packet = await asyncio.wait_for(peer.queue.get(), .5)
            except asyncio.TimeoutError:
                continue
            if (self.valid(source) and self.valid(peer)
                    and self.accounts.directory.voice_blocks.allowed(source.player, peer.player)):
                # A slow reader never stalls the microphone or other listeners.
                try:
                    await asyncio.wait_for(peer.socket.send_bytes(packet), .25)
                except (asyncio.TimeoutError, ConnectionError):
                    await peer.socket.close(code=1011)
                    return

    async def connect(self, request):
        key, who = self.accounts.session(request)
        member = self.member(who)
        require(member is not None, "Join an available "+self.group_name.lower()+" before joining voice", 403)
        require(len(self.peers) < 256 or who in self.peers, "Voice service is full", 503)
        require(sum(p.clan == member[0] and p.player != who for p in self.peers.values()) < 16,
                self.group_name+" voice is full (16 players)", 409)
        socket = web.WebSocketResponse(heartbeat=10, max_msg_size=262, compress=False)
        await socket.prepare(request)
        peer = Peer(key, who, member[0], socket)
        # Recheck admission after the asynchronous HTTP upgrade.
        if (not self.valid(peer) or (len(self.peers) >= 256 and who not in self.peers)
                or sum(p.clan == peer.clan and p.player != who for p in self.peers.values()) >= 16):
            await socket.close(code=1008)
            return socket
        old = self.peers.get(who)
        self.peers[who] = peer
        if old:
            await old.socket.close(code=1008, message=b"Voice connected elsewhere")
        sender = asyncio.create_task(self.writer(peer))
        credits, previous, sequence = 10., time.monotonic(), -1
        try:
            async for message in socket:
                if not self.valid(peer) or self.peers.get(who) is not peer:
                    await socket.close(code=1008)
                    break
                if message.type != WSMsgType.BINARY:
                    await socket.close(code=1003)
                    break
                packet = message.data
                now = time.monotonic()
                credits = min(10., credits + (now - previous) * 50.)
                previous = now
                # version + monotonic u32 sequence + one 20ms Opus packet.
                value = int.from_bytes(packet[1:5], "big")
                if not 6 <= len(packet) <= 261 or packet[0] != 1 or value <= sequence or credits < 1:
                    await socket.close(code=1008, message=b"Invalid voice frame")
                    break
                sequence = value
                credits -= 1
                delivered = packet[:1] + uuid.UUID(self.identity(who)).bytes + packet[1:]
                for listener in tuple(self.peers.values()):
                    if (listener is peer or listener.clan != peer.clan or not self.valid(listener)
                            or not self.accounts.directory.voice_blocks.allowed(who, listener.player)):
                        continue
                    if listener.queue.full():
                        listener.queue.get_nowait()
                    listener.queue.put_nowait((peer, delivered))
        finally:
            if self.peers.get(who) is peer:
                del self.peers[who]
            sender.cancel()
            with contextlib.suppress(asyncio.CancelledError, ConnectionError, asyncio.TimeoutError):
                await sender
            await socket.close()
        return socket

    async def close(self, app):
        await asyncio.gather(*(p.socket.close(code=1001) for p in tuple(self.peers.values())))


class PartyVoice(ClanVoice):
    """Passive accepted-party audience outside an active gameplay session.

    Authentication, bounded queues and codec framing reuse the native clan
    relay. Membership and wire identities come only from accepted parties.
    """
    group_name = "Party"

    def __init__(self, accounts, parties):
        super().__init__(accounts)
        self.parties = parties

    def member(self, who):
        if self.parties.playing(who):
            return None
        party = self.parties.party(who)
        return party[:2] if party else None

    def identity(self, who):
        return self.parties.db.execute("SELECT handle FROM party_members WHERE player=?", (who,)).fetchone()[0]
