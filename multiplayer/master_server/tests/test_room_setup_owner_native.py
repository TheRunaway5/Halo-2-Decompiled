"""The original start clock gates running even after early loader reports."""
import asyncio
import unittest
import time
from . import test_match_transport as baseline


class RoomSetupOwnerTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = baseline.MatchTransportTests.asyncSetUp
    session = baseline.MatchTransportTests.session
    enqueue = baseline.MatchTransportTests.enqueue
    pair = baseline.MatchTransportTests.pair

    async def test_real_service_clock_waits_ten_seconds_before_running(self):
        self.real_start_clock = True
        _, descriptors, sockets, queues, _ = await self.pair()
        room = self.api.transport.rooms[descriptors[0]['id']]
        before = time.monotonic()
        for socket in sockets:
            await socket.send_json({'loaded':'1234567890abcdef:000001a'})
        async with asyncio.timeout(2):
            while len(room.loaded) != len(room.tokens):
                await asyncio.sleep(.005)
        self.assertEqual(room.phase, 'loading')
        self.assertGreater(room.start_countdown_ms(), 0)
        for queue in queues:
            self.assertEqual((await asyncio.wait_for(queue.get(), 15))['phase'], 'running')
        self.assertGreaterEqual(time.monotonic()-before, 9.5)

    async def test_setup_observations_are_read_only_and_remain_after_auth_retirement(self):
        headers, descriptors, _, _, _ = await self.pair()
        room = self.api.transport.rooms[descriptors[0]['id']]
        owner = next(iter(room.samples))
        with self.assertRaises(TypeError): room.samples[owner] = ()
        with self.assertRaises(AttributeError): room.start_clock = 0
        response = await self.client.delete('/v1/matchmaking/session', headers=headers[0])
        self.assertEqual(response.status, 200, await response.text())
        self.assertTrue(room.samples)
        self.assertEqual(room.phase, 'loading')

    async def test_early_loaded_reports_cannot_skip_the_original_start_clock(self):
        _, descriptors, sockets, queues, _ = await self.pair()
        room = self.api.transport.rooms[descriptors[0]['id']]
        started = room.start_clock
        now = [started + 5000]
        room.clock = lambda: now[0]
        for socket in sockets:
            await socket.send_json({'loaded':'1234567890abcdef:000001a'})
        async with asyncio.timeout(2):
            while len(room.loaded) != len(room.tokens):
                await asyncio.sleep(.005)
        self.assertEqual(room.phase, 'loading', 'Early loader reports bypassed the original ten-second countdown')
        self.assertEqual(room.start_countdown_ms(), 5000)
        now[0] = started + 10000
        await room.advance()
        for queue in queues:
            self.assertEqual((await asyncio.wait_for(queue.get(), 2))['phase'], 'running')
