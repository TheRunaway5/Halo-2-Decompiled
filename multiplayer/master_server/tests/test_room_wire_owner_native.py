"""Native capability/signaling authority over actual HTTP setup fixtures."""
import unittest
from aiohttp import WSServerHandshakeError
from . import test_direct_peer_http as baseline


class RoomWireOwnerTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = baseline.DirectPeerHttpTests.asyncSetUp
    session = baseline.DirectPeerHttpTests.session
    enqueue = baseline.DirectPeerHttpTests.enqueue
    descriptors = baseline.DirectPeerHttpTests.descriptors
    connect = baseline.DirectPeerHttpTests.connect

    async def test_expired_match_is_rejected_before_websocket_upgrade(self):
        rows = await self.descriptors()
        room = self.api.transport.rooms[rows[0]['id']]
        expired = room.deadline + 1
        room.lifecycle_clock = lambda: expired
        with self.assertRaises(WSServerHandshakeError) as caught:
            socket = await self.client.ws_connect('/v1/matches/'+room.id+'/signal',
                headers={'Authorization':'Bearer '+rows[0]['token']})
            await socket.close()
        self.assertEqual(caught.exception.status, 404)

    async def test_capabilities_are_native_read_only_and_rotation_rejects_old_bearer(self):
        rows = await self.descriptors()
        room = self.api.transport.rooms[rows[0]['id']]
        owner = room.players[rows[0]['slot']][0]
        with self.assertRaises(TypeError): room.tokens[owner] = 'forged'
        token = room.room_call('room_wire_rotate', player=owner)['token']
        self.assertNotEqual(token, rows[0]['token'])
        with self.assertRaises(WSServerHandshakeError) as caught:
            await self.client.ws_connect('/v1/matches/'+room.id+'/signal',
                headers={'Authorization':'Bearer '+rows[0]['token']})
        self.assertEqual(caught.exception.status, 401)
        socket, _ = await self.connect(rows[0] | {'token':token})
        self.assertFalse(socket.closed)
        with self.assertRaises(AttributeError): room.peers[owner].connection = 0

    async def test_duplicate_connecting_peer_is_rejected_before_upgrade(self):
        rows = await self.descriptors()
        socket, _ = await self.connect(rows[0])
        room = self.api.transport.rooms[rows[0]['id']]
        owner = room.players[rows[0]['slot']][0]
        generation = room.peers[owner].connection
        with self.assertRaises(WSServerHandshakeError) as caught:
            await self.client.ws_connect('/v1/matches/'+room.id+'/signal',
                headers={'Authorization':'Bearer '+rows[0]['token']})
        self.assertEqual(caught.exception.status, 409)
        self.assertEqual(room.peers[owner].connection, generation)
        self.assertFalse(socket.closed)
