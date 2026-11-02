"""Temporary WebSocket/audio IO for the Rust private social voice relay.

Rust owns current authorization, memberships, generations, framing, budgets,
queues and delivery. Python retains sockets, wake events and send deadlines.
"""
import asyncio
import contextlib
import time
from aiohttp import web, WSMsgType
from .protocol import Problem
from .native_social_voice import Relay
from .native_party_directory import run


class Peer:
    def __init__(self,identifier,socket):
        self.id,self.socket=identifier,socket
        self.wake=asyncio.Event()


class ClanVoice:
    kind='clan'
    def __init__(self,accounts):
        self.accounts,self.directory=accounts,accounts.directory
        self.db,self.wall,self.clock=accounts.db,accounts.wall,time.monotonic
        self.peers={}
        self.relay=Relay(self,self.kind)
        run(self,'relay_schema',family='voice')

    def call(self,mode,**body):return run(self,mode,family='voice',body=body)

    async def writer(self,peer):
        try:
            while not peer.socket.closed:
                peer.wake.clear()
                result=self.call('poll',peer=peer.id)
                if result.get('close'):
                    await peer.socket.close(code=result['code'])
                    return
                if result.get('packet'):
                    await asyncio.wait_for(peer.socket.send_bytes(bytes.fromhex(result['packet'])),.25)
                elif not result.get('again'):
                    try:await asyncio.wait_for(peer.wake.wait(),.5)
                    except asyncio.TimeoutError:pass
        except Problem:
            await peer.socket.close(code=1008)
        except (asyncio.TimeoutError,ConnectionError):
            await peer.socket.close(code=1011)

    async def connect(self,request):
        key,who=self.accounts.session(request)
        run(self,'preflight',who,family='voice',account_key=key)
        socket=web.WebSocketResponse(heartbeat=10,max_msg_size=262,compress=False)
        await socket.prepare(request)
        try:attached=run(self,'attach',who,family='voice',account_key=key)
        except Problem:
            await socket.close(code=1008)
            return socket
        peer=Peer(attached['peer'],socket)
        self.peers[peer.id]=peer
        old=self.peers.get(attached['replaced'])
        if old:await old.socket.close(code=1008,message=b'Voice connected elsewhere')
        sender=asyncio.create_task(self.writer(peer))
        try:
            async for message in socket:
                result=self.call('frame',peer=peer.id,binary=message.type==WSMsgType.BINARY,
                                 packet=message.data.hex() if message.type==WSMsgType.BINARY else '')
                if result.get('close'):
                    await socket.close(code=result['code'])
                    break
                for identifier in result.get('wake',()):
                    listener=self.peers.get(identifier)
                    if listener:listener.wake.set()
        except Problem:
            await socket.close(code=1008)
        finally:
            self.peers.pop(peer.id,None)
            if self.relay._handle:
                with contextlib.suppress(Problem):self.call('detach',peer=peer.id)
            sender.cancel()
            with contextlib.suppress(asyncio.CancelledError,ConnectionError,asyncio.TimeoutError):await sender
            await socket.close()
        return socket

    async def close(self,app):
        await asyncio.gather(*(p.socket.close(code=1001) for p in tuple(self.peers.values())))


class PartyVoice(ClanVoice):
    kind='party'
    def __init__(self,accounts,parties):
        super().__init__(accounts)
