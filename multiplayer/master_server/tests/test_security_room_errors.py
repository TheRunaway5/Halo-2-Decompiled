import asyncio
import unittest
from aiohttp import WSServerHandshakeError
from . import test_direct_peer_http as direct


class DirectRoomErrorTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = direct.DirectStarConnectivityTests.asyncSetUp
    session = direct.DirectStarConnectivityTests.session
    enqueue = direct.DirectStarConnectivityTests.enqueue
    connect = direct.DirectStarConnectivityTests.connect
    next = direct.DirectStarConnectivityTests.next
    catalog_fixture = direct.DirectStarConnectivityTests.catalog_fixture
    running_three = direct.DirectStarConnectivityTests.running_three

    async def check_offender(self, value):
        room, descriptors, connections, host = await self.running_three()
        bad = next(i for i in range(3) if i != host)
        if isinstance(value, dict):
            await connections[bad][0].send_json(value)
        elif isinstance(value, str):
            await connections[bad][0].send_str(value)
        else:
            await connections[bad][0].send_bytes(value)
        self.assertIsNone(await self.next(connections[bad][1], 'closed'))
        self.assertFalse(room.closed)
        self.assertEqual(room.phase, 'running')
        self.assertEqual(len(room.tokens), 2)
        for i in range(3):
            if i != bad:
                self.assertFalse(connections[i][0].closed)
        with self.assertRaises(WSServerHandshakeError) as error:
            await self.client.ws_connect('/v1/matches/' + room.id + '/signal',
                headers={'Authorization': 'Bearer ' + descriptors[bad]['token']})
        self.assertEqual(error.exception.status, 401)

    async def test_unknown_control_retires_only_the_nonhost(self):
        await self.check_offender({'unknown': True})

    async def test_malformed_json_retires_only_the_nonhost(self):
        await self.check_offender('{invalid')

    async def test_binary_media_on_signaling_retires_only_the_nonhost(self):
        await self.check_offender(b'H2N2\x42\0\x06' + b'x' * 32)

    async def test_trusted_coordinator_shutdown_still_closes_the_room(self):
        room, descriptors, connections, host = await self.running_three()
        await room.close('Coordinator shutdown fixture')
        for _, queue in connections:
            self.assertIsNone(await self.next(queue, 'closed'))
        self.assertTrue(room.closed)
