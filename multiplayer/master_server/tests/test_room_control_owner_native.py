"""Authenticated HTTP controls reach the frozen native room owner."""
import asyncio
import json
import unittest
from . import test_match_transport as baseline


class RoomControlOwnerTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = baseline.MatchTransportTests.asyncSetUp
    session = baseline.MatchTransportTests.session
    enqueue = baseline.MatchTransportTests.enqueue
    pair = baseline.MatchTransportTests.pair

    async def test_duplicate_decoded_control_keys_cannot_replace_loaded_admission(self):
        _, descriptors, sockets, queues, _ = await self.pair()
        await sockets[0].send_str('{"loaded":"not-an-admission","lo\\u0061ded":"1234567890abcdef:000001c"}')
        await sockets[1].send_json({'loaded':'1234567890abcdef:000001c'})
        value = await asyncio.wait_for(queues[1].get(),2)
        self.assertIsNone(value,'Duplicate decoded controls silently replaced a loaded admission field')

    async def test_python_challenge_aliases_cannot_authorize_a_pong(self):
        _, descriptors, sockets, queues, _ = await self.pair()
        room = self.api.transport.rooms[descriptors[0]['id']]
        player = room.players[descriptors[0]['slot']][0]
        peer = room.peers[player]
        peer.challenge, peer.sent = 'f'*16, peer.control_clock()
        await sockets[0].send_json({'pong':peer.challenge})
        self.assertIsNone(await asyncio.wait_for(queues[1].get(),2))
        self.assertTrue(room.closed)


from . import test_direct_peer_http as direct


class DirectProbeOwnerTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = direct.DirectPeerHttpTests.asyncSetUp
    session = direct.DirectPeerHttpTests.session
    enqueue = direct.DirectPeerHttpTests.enqueue
    descriptors = direct.DirectPeerHttpTests.descriptors
    connect = direct.DirectPeerHttpTests.connect
    next = direct.DirectPeerHttpTests.next

    async def test_late_pong_does_not_add_qos_or_revoke_direct_gameplay_membership(self):
        rows = await self.descriptors()
        connections = [await self.connect(d) for d in rows]
        for descriptor,(socket,_) in zip(rows,connections):
            other = next(d for d in rows if d['slot'] != descriptor['slot'])
            await socket.send_json({'peer_ready':[dict(peer=other['slot'],rtt_ms=20)]})
        for socket,queue in connections:
            await self.next(queue,'phase','loading')
            await socket.send_json({'loaded':'1234567890abcdef:0000042'})
        for _,queue in connections:
            await self.next(queue,'phase','running')
        room = self.api.transport.rooms[rows[0]['id']]
        player = room.players[rows[0]['slot']][0]
        peer = room.peers[player]
        samples = room.samples[player]
        now = peer.control_clock()+6.
        peer.control_clock = lambda:now
        effect = room.control_call('room_control_tick',player,peer)
        nonce = next(p['packet']['ping'] for p in effect['publications'] if 'ping' in p['packet'])
        now += 15.001
        await connections[0][0].send_json({'pong':nonce})
        self.assertIsNone(await self.next(connections[0][1],'phase'))
        self.assertEqual(room.samples[player],samples)
        self.assertFalse(room.closed)
        self.assertTrue(room.active_member(player))
        self.assertIn(player,room.tokens)
        self.assertEqual(json.loads(room.voice_membership())['voice_members'],3)
