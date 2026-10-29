import asyncio
import json
import unittest

from aiohttp import WSServerHandshakeError, WSMsgType
from . import test_matchmaking_http as http_fixture


class SearchQosTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = http_fixture.MatchmakingHttpTests.asyncSetUp
    enqueue = http_fixture.MatchmakingHttpTests.enqueue

    async def session(self, owner="one", *, compatibility="halo2-native-queue-v8", **extra):
        body = dict(screen_name="Private chosen name", maps=["lockout"], compatibility=compatibility, language=0)
        response = await self.client.post("/v1/matchmaking/session",
            headers={"Authorization": "Bearer "+owner}, json=body | extra)
        self.assertEqual(response.status, 201, await response.text())
        return {"Authorization": "Bearer "+(await response.json())["token"]}

    async def connect(self, headers):
        socket = await self.client.ws_connect("/v1/matchmaking/qos", headers=headers)
        self.addAsyncCleanup(socket.close)
        return socket

    async def receive(self, socket):
        while True:
            message = await asyncio.wait_for(socket.receive(), 1)
            self.assertEqual(message.type, WSMsgType.TEXT)
            value = json.loads(message.data)
            self.assertEqual(len(value), 1)
            if "ping" in value:
                await socket.send_json({"pong": value["ping"]})
                continue
            self.assertEqual(set(value), {"probe"})
            self.assertEqual(len(value["probe"]), 32)
            return value["probe"]

    async def pair(self):
        headers = [await self.session(), await self.session("two")]
        qos = self.api.search_qos
        self.now = 10.
        qos.clock = lambda: self.now
        for header in headers:
            self.assertEqual((await self.enqueue(header)).status, 201)
        sockets = [await self.connect(header) for header in headers]
        # The route starts at the lexically first private ID. It is never
        # disclosed to either socket; order is only inspected inside this test.
        ids = [self.api.session(type("Request", (), {"headers": h})())[1].player for h in headers]
        ordered = sorted(zip(ids, sockets))
        return qos, headers, ids, [s for _, s in ordered]

    async def test_actual_two_socket_cycle_three_samples_median_and_expiry(self):
        qos, _, ids, sockets = await self.pair()
        qos.request(*ids)
        for total in (30, 90, 50):
            first = await self.receive(sockets[0])
            probe = qos.probes[tuple(sorted(ids))]
            self.assertFalse(probe.second)
            start = probe.started
            # Neither independently reported RTT nor a single socket's reply
            # can make a candidate ready. Both actual sockets must be visited.
            self.assertIsNone(qos.measurement(*ids))
            self.now = start+.01
            await sockets[0].send_json({"echo": first})
            second = await self.receive(sockets[1])
            self.assertNotEqual(first, second)
            self.assertIsNone(qos.measurement(*ids))
            self.now = start+(total+.1)/1000
            await sockets[1].send_json({"echo": second})
        async with asyncio.timeout(1):
            while qos.measurement(*ids) is None:
                await asyncio.sleep(.001)
        self.assertEqual(qos.measurement(*ids).milliseconds, 50)
        self.assertEqual(qos.measurement(*reversed(ids)), qos.measurement(*ids))
        self.assertFalse(qos.samples)
        self.assertFalse(qos.probes)
        self.now += 31
        self.assertIsNone(qos.measurement(*ids))

    async def test_disconnect_and_queue_cancellation_invalidate_measurements(self):
        qos, headers, ids, sockets = await self.pair()
        qos.request(*ids)
        for total in (30,90,50):
            first=await self.receive(sockets[0])
            start=qos.probes[tuple(sorted(ids))].started
            self.now=start+.01
            await sockets[0].send_json({'echo':first})
            second=await self.receive(sockets[1])
            self.now=start+(total+.1)/1000
            await sockets[1].send_json({'echo':second})
        async with asyncio.timeout(1):
            while qos.measurement(*ids) is None:await asyncio.sleep(.001)
        self.assertIsNotNone(qos.measurement(*ids))
        await sockets[0].close()
        self.assertIsNone(qos.measurement(*ids))
        self.assertFalse(qos.measurements)
        response = await self.client.delete("/v1/matchmaking/ticket", headers=headers[1])
        self.assertEqual(response.status, 200)
        qos.request(*ids)
        self.assertFalse(qos.probes)

    async def test_identity_binding_anonymous_old_version_and_duplicate_denied(self):
        with self.assertRaises(WSServerHandshakeError) as error:
            await self.client.ws_connect("/v1/matchmaking/qos")
        self.assertEqual(error.exception.status, 401)
        header = await self.session(compatibility="halo2-native-queue-v7")
        with self.assertRaises(WSServerHandshakeError) as error:
            await self.client.ws_connect("/v1/matchmaking/qos", headers=header)
        self.assertEqual(error.exception.status, 409)
        header = await self.session()
        await self.connect(header)
        with self.assertRaises(WSServerHandshakeError) as error:
            await self.client.ws_connect("/v1/matchmaking/qos", headers=header)
        self.assertEqual(error.exception.status, 409)

    async def test_clients_cannot_assert_targets_measurements_or_replay_challenges(self):
        for payload in ({"milliseconds": 1}, {"echo": "0"*32},
                        {"echo": "0"*32, "target": "other-player"}, {"rank": 127}):
            header = await self.session()
            socket = await self.connect(header)
            await socket.send_json(payload)
            async with asyncio.timeout(1):
                async for _ in socket:
                    pass
            self.assertFalse(self.api.search_qos.measurements)
            # Session rotation also ensures no race with the old socket's
            # teardown can install a new peer under the revoked token.
            await self.client.delete("/v1/matchmaking/session", headers=header)

    async def test_logout_revokes_open_search_socket(self):
        header = await self.session()
        socket = await self.connect(header)
        response = await self.client.delete("/v1/matchmaking/session", headers=header)
        self.assertEqual(response.status, 200)
        async with asyncio.timeout(1):
            async for _ in socket:
                pass
        self.assertFalse(self.api.search_qos.peers)

    async def test_probe_views_and_live_reservations_are_native_read_only(self):
        qos, _, ids, _ = await self.pair()
        with self.assertRaises(AttributeError):qos.peers[ids[0]].probe_credits=0
        with self.assertRaises(TypeError):qos.measurements[tuple(sorted(ids))]=None
        self.assertTrue(qos.request(*ids))
        self.assertEqual(len(qos.probes),1)
        self.assertTrue(qos.request(*ids))
        self.assertEqual(len(qos.probes),1)
        # Exact reservation depletion/refill and recovery are qualified by
        # native controls; no mutable Python budget is installed in a peer.

    async def test_native_language_is_required_bounded_and_private(self):
        for language in (None, True, -1, 9, "en-US"):
            response = await self.client.post("/v1/matchmaking/session", headers={"Authorization": "Bearer one"},
                json=dict(screen_name="Chosen name", maps=["lockout"], compatibility="halo2-native-queue-v8",
                          language=language))
            self.assertEqual(response.status, 400)
        header = await self.session(language=6)
        response = await self.client.get("/v1/matchmaking/playlists", headers=header)
        self.assertNotIn('"language"', await response.text())
        self.assertEqual(next(iter(self.api.sessions.values())).language, 6)
