"""Temporary socket IO for Rust-owned relay-cycle QoS.

Rust owns session/generation guards, physical pairs, clocks/nonces, reservations,
probe delivery, samples/medians, retry work, expiry and limits. This native relay
cycle is distinct from original Xbox XNet/direct-peer QoS and service recovery.
"""
import asyncio
import contextlib
import json
import time
from collections.abc import Mapping
from dataclasses import dataclass
from aiohttp import web, WSMsgType
from .protocol import require


@dataclass(frozen=True)
class Measurement:
    milliseconds: int
    expires: float


@dataclass(frozen=True)
class Probe:
    pair: tuple[str,str]
    nonce: str
    expires: float
    waiting: str
    started: float | None
    second: bool


@dataclass(frozen=True)
class Samples:
    expires: float
    values: tuple[int,...]


class _View(Mapping):
    def __init__(self,owner,table):self.owner,self.table=owner,table
    def __len__(self):return self.owner._call('stats')[self.table]
    def __iter__(self):
        offset=0
        while True:
            rows=self.owner._call('keys',table=self.table,offset=offset)
            if not rows:return
            yield from (tuple(pair) for pair in rows)
            offset+=len(rows)
    def __getitem__(self,pair):
        value=self.owner._call('get',table=self.table,a=pair[0],b=pair[1])
        if value is None:raise KeyError(pair)
        if self.table=='measurements':return Measurement(**value)
        if self.table=='samples':return Samples(value['expires'],tuple(value['values']))
        return Probe(**(value|dict(pair=tuple(value['pair']))))


class Peer:
    __slots__=('socket','_connection','_player','wakeup')
    def __init__(self,socket,admission):
        self.socket=socket
        self._connection,self._player=admission['connection'],admission['player']
        self.wakeup=asyncio.Event()
    @property
    def connection(self):return self._connection
    @property
    def player(self):return self._player


class SearchQos:
    def __init__(self,api,*,clock=time.monotonic,lease=30,timeout=4):
        self.api,self.clock,self.peers=api,clock,{}
        self.probes,self.measurements,self.samples=(_View(self,t) for t in ('probes','measurements','samples'))
        if api.core is not None:
            self._call('init',lease=lease,timeout=timeout)
        api.search_qos=self

    def _call(self,op,**values):
        require(self.api.core is not None,'Matchmaking is not configured',503)
        result=self.api.core._call('search_qos_'+op,qos_now=self.clock(),
            io_peers=[dict(player=who,connection=p.connection,closed=p.socket.closed) for who,p in self.peers.items()],**values)
        while True:
            wakes=self.api.core._call('search_qos_wakeups')
            for wake in wakes:
                peer=self.peers.get(wake['player'])
                if peer is not None and peer.connection==wake['connection']:
                    peer.wakeup.set()
            if len(wakes)<32:break
        return result

    @property
    def lease(self):return self._call('stats')['lease']
    @property
    def timeout(self):return self._call('stats')['timeout']

    def discard(self,player,*,generation=None):
        self._call('discard',player=player,generation=generation)

    def cleanup(self):self._call('cleanup')
    def ready(self,player):return self._call('ready',player=player) if self.api.core else False
    def measurement(self,a,b):
        value=self._call('measurement',a=a,b=b)
        return Measurement(**value) if value is not None else None
    def request(self,a,b):return self._call('request',a=a,b=b)
    def pending(self,a,b):return self._call('pending',a=a,b=b)
    def echo(self,player,nonce):
        return self._call('receive',player=player,connection=self.peers[player].connection,
                          kind='text',text=json.dumps(dict(echo=nonce)))

    async def writer(self,player,peer):
        try:
            while True:
                peer.wakeup.clear()
                effect=self._call('tick',player=player,connection=peer.connection)
                if effect['close']:break
                for message in effect['messages']:
                    await asyncio.wait_for(peer.socket.send_json(message),.5)
                try:await asyncio.wait_for(peer.wakeup.wait(),.05)
                except asyncio.TimeoutError:pass
        finally:
            await peer.socket.close()

    async def connect(self,request):
        key,session=self.api.session(request)
        source=dict(key=key,generation=session.generation)
        admission=self._call('authorize',**source,prior=None)
        socket=web.WebSocketResponse(max_msg_size=256,compress=False,autoping=True)
        await socket.prepare(request)
        try:
            attached=self._call('attach',**source,prior=admission)
        except Exception:
            await socket.close()
            return socket
        peer=Peer(socket,attached)
        previous=self.peers.get(peer.player)
        self.peers[peer.player]=peer
        if previous:
            await previous.socket.close()
        writer=asyncio.create_task(self.writer(peer.player,peer))
        try:
            async for message in socket:
                if message.type in (WSMsgType.CLOSE,WSMsgType.CLOSED,WSMsgType.ERROR):break
                self._call('receive',player=peer.player,connection=peer.connection,
                    kind=message.type.name.lower(),text=message.data if message.type==WSMsgType.TEXT else None)
        except Exception:
            # No payload, nonce, identity, bearer or observation enters logs.
            pass
        finally:
            self._call('detach',player=peer.player,connection=peer.connection)
            if self.peers.get(peer.player) is peer:self.peers.pop(peer.player)
            writer.cancel()
            with contextlib.suppress(asyncio.CancelledError,ConnectionError,asyncio.TimeoutError):await writer
            await socket.close()
        return socket

    async def close(self,app):
        await asyncio.gather(*(p.socket.close() for p in tuple(self.peers.values())))

    def routes(self):return [web.get('/v1/matchmaking/qos',self.connect)]
