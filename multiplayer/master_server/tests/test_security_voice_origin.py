import asyncio
import json
import unittest
from dataclasses import replace
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from multiplayer.master_server.voice_origin import voice_message
from . import test_match_transport as match
from . import test_couch_online as couch
from .voice_signing import sign_voice


class VoiceOriginTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = match.MatchTransportTests.asyncSetUp
    session = match.MatchTransportTests.session
    enqueue = match.MatchTransportTests.enqueue
    pair = match.MatchTransportTests.pair

    async def running(self):
        self.api.entries = tuple(replace(e, policy=replace(e.policy, minimum_players=3, maximum_players=3)) for e in self.api.entries)
        self.api.core.playlists = {e.policy.id: e.policy for e in self.api.entries}
        _, descriptors, sockets, queues, host = await self.pair(('one', 'two', 'three'))
        for socket in sockets:
            await socket.send_json({'loaded': '1234567890abcdef:000001c'})
        for queue in queues:
            self.assertEqual((await asyncio.wait_for(queue.get(), 2))['phase'], 'running')
        room = self.api.transport.rooms[descriptors[0]['id']]
        h = next(i for i, d in enumerate(descriptors) if d['slot'] == host)
        a, b = [i for i in range(3) if i != h]
        return room, descriptors, sockets, queues, h, a, b

    def body(self, slot):
        return b'H2N2\x1c\0\x06\0' + bytes([slot]) + b'\1\0\0\0\0\0\0\x80\x3f\0\0\x80\x3f\x42'

    async def test_host_cannot_sign_another_microphones_identity(self):
        room, descriptors, sockets, queues, h, a, b = await self.running()
        body = self.body(descriptors[a]['slot'])
        host_owner = room.players[descriptors[h]['slot']][0]
        forged = sign_voice(room, body, owner=host_owner)
        await sockets[h].send_bytes(bytes([descriptors[b]['slot']]) + forged)
        self.assertIsNone(await asyncio.wait_for(queues[b].get(), 2))
        self.assertTrue(room.closed)

    async def test_unsigned_host_audio_cannot_claim_another_microphone(self):
        room, descriptors, sockets, queues, h, a, b = await self.running()
        await sockets[h].send_bytes(bytes([descriptors[b]['slot']]) + self.body(descriptors[a]['slot']))
        self.assertIsNone(await asyncio.wait_for(queues[b].get(), 2))
        self.assertTrue(room.closed)

    async def test_valid_origin_survives_host_forwarding_and_gain_mixing(self):
        room, descriptors, sockets, queues, h, a, b = await self.running()
        packet = sign_voice(room, self.body(descriptors[a]['slot']))
        await sockets[a].send_bytes(bytes([descriptors[h]['slot']]) + packet)
        self.assertEqual(await asyncio.wait_for(queues[h].get(), 2), bytes([descriptors[a]['slot']]) + packet)
        mixed = bytearray(packet)
        mixed[14:18] = b'\0\0\0\0'
        await sockets[h].send_bytes(bytes([descriptors[b]['slot']]) + mixed)
        self.assertEqual(await asyncio.wait_for(queues[b].get(), 2), bytes([descriptors[h]['slot']]) + mixed)
        self.assertFalse(room.closed)

    async def test_other_room_signature_cannot_be_replayed(self):
        room, descriptors, sockets, queues, h, a, b = await self.running()
        body = self.body(descriptors[a]['slot'])
        owner = room.owners[room.players[descriptors[a]['slot']][0]]
        packet = body + room.voice_keys[owner].sign(voice_message('00000000-0000-0000-0000-000000000000', 0, body))
        await sockets[h].send_bytes(bytes([descriptors[b]['slot']]) + packet)
        self.assertIsNone(await asyncio.wait_for(queues[b].get(), 2))

    async def test_connection_receives_only_its_own_room_signing_seed(self):
        room, descriptors, sockets, queues, h, a, b = await self.running()
        seeds = set()
        for descriptor in descriptors:
            owner = room.players[descriptor['slot']][0]
            auth = json.loads(room.voice_auth(owner))['voice_auth']
            seeds.add(auth['seed'])
            key = Ed25519PrivateKey.from_private_bytes(bytes.fromhex(auth['seed']))
            self.assertEqual(key.public_key().public_bytes_raw().hex(), auth['keys'][descriptor['slot']])
            self.assertEqual(auth, json.loads(room.voice_auth(owner))['voice_auth'])
        self.assertEqual(len(seeds), 3)


class CouchVoiceOriginTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = couch.CouchOnlineTests.asyncSetUp
    catalog_fixture = staticmethod(couch.catalog)
    session = couch.CouchOnlineTests.session
    enqueue = couch.CouchOnlineTests.enqueue
    pair = match.MatchTransportTests.pair

    async def test_guest_seat_uses_its_physical_owners_microphone_key(self):
        _, descriptors, sockets, queues, host = await self.pair()
        for socket in sockets:
            await socket.send_json({'loaded': '1234567890abcdef:0000035'})
        for queue in queues:
            self.assertEqual((await asyncio.wait_for(queue.get(), 2))['phase'], 'running')
        h = next(i for i, d in enumerate(descriptors) if d['slot'] == host)
        sender = 1 - h
        slot = descriptors[sender]['local_slots'][1]
        room = self.api.transport.rooms[descriptors[0]['id']]
        body = b'H2N2\x35\0\x06\0' + bytes([slot]) + b'\0' * 14
        packet = sign_voice(room, body)
        await sockets[sender].send_bytes(bytes([host]) + packet)
        self.assertEqual(await asyncio.wait_for(queues[h].get(), 2), bytes([descriptors[sender]['slot']]) + packet)
        self.assertEqual(len(room.voice_keys), 2)
        owner = room.owners[room.players[slot][0]]
        auth = json.loads(room.voice_auth(owner))['voice_auth']
        self.assertEqual(auth['keys'][slot], auth['keys'][descriptors[sender]['slot']])
        forged = sign_voice(room, body, owner=room.players[host][0])
        await sockets[h].send_bytes(bytes([descriptors[sender]['slot']]) + forged)
        self.assertIsNone(await asyncio.wait_for(queues[sender].get(), 2))
