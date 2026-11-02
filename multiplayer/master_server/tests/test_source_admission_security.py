"""Source rotation must not consume unrelated limiter/login capacity."""
import base64
import asyncio
import hashlib
import unittest
from collections import OrderedDict
from types import SimpleNamespace
from unittest.mock import patch

from multiplayer.master_server.protocol import Problem
from multiplayer.master_server.service import Directory, Settings
from multiplayer.master_server.request_admission import TokenBuckets
from . import test_steam_login as steam


class LimiterCapacityTests(unittest.TestCase):
    def test_full_source_table_does_not_reject_unseen_clients_or_principals(self):
        directory = Directory.__new__(Directory)
        directory.settings = Settings()
        directory.buckets = TokenBuckets()
        for index in range(4096):
            directory.rate('ip:2001:db8::' + format(index, 'x'))
        directory.rate('ip:2001:db9::1')
        directory.rate('owner-id:steam:legitimate', 20)

    def test_one_ipv6_prefix_is_throttled_before_exact_cache_growth(self):
        directory = Directory.__new__(Directory)
        directory.settings, directory.buckets = Settings(), TokenBuckets()
        with patch('multiplayer.master_server.request_admission.time.monotonic', return_value=1000):
            accepted = 0
            for index in range(4097):
                try:
                    directory.admit_source(SimpleNamespace(remote='2001:db8::'+format(index+1, 'x'), headers={}))
                    accepted += 1
                except Problem as error:
                    self.assertEqual(error.status, 429)
            directory.admit_source(SimpleNamespace(remote='2001:db9::1', headers={}))
        self.assertEqual(accepted, 512)
        self.assertEqual(len(directory.buckets.tables['source']), 513)

    def test_source_churn_does_not_refill_principal_or_peer_budgets(self):
        buckets = TokenBuckets()
        with patch('multiplayer.master_server.request_admission.time.monotonic', return_value=1000):
            buckets.charge('owner-id:steam:real', 1)
            buckets.charge('peer:pinned', 1, interval=3600)
            for index in range(8192):
                buckets.charge('ip:'+str(index), 600)
            for key in ('owner-id:steam:real', 'peer:pinned'):
                with self.assertRaises(Problem) as error:
                    buckets.charge(key, 1)
                self.assertEqual(error.exception.status, 429)
        self.assertEqual(len(buckets.tables['source']), 4096)
        self.assertEqual(buckets.evictions, 4096)

    def test_full_table_admission_never_sweeps_entries(self):
        class NoSweep(OrderedDict):
            def __iter__(self):
                raise AssertionError('Full-table scan')
            def items(self):
                raise AssertionError('Full-table scan')
        buckets = TokenBuckets()
        for index in range(4096):
            buckets.charge('ip:'+str(index), 600)
        buckets.tables['source'] = NoSweep(buckets.tables['source'])
        for index in range(1000):
            buckets.charge('ip:new-'+str(index), 600)
        self.assertEqual(buckets.evictions, 1000)

    def test_configured_prefix_and_ipv4_mapping(self):
        from multiplayer.master_server.request_admission import source_prefix
        self.assertEqual(source_prefix('::ffff:203.0.113.1'), source_prefix('203.0.113.1'))
        self.assertEqual(source_prefix('2001:db8:0:1::1', 56), source_prefix('2001:db8:0:2::1', 56))


class SteamStartCapacityTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = steam.SteamLoginTests.asyncSetUp
    start = steam.SteamLoginTests.start
    assertion = steam.SteamLoginTests.assertion
    signature = steam.SteamLoginTests.signature

    async def test_rotating_one_prefix_cannot_fill_login_bank(self):
        device = await self.start()
        response = await self.client.get('/v1/auth/steam/callback/'+device,
            params=self.assertion(device), allow_redirects=False)
        self.assertEqual(response.status, 303)
        digest = base64.urlsafe_b64encode(hashlib.sha256(self.verifier.encode()).digest()).rstrip(b'=').decode()
        accepted = 0
        for index in range(1001):
            with patch.object(self.directory, 'source', return_value='2001:db8::'+format(index+1, 'x')):
                response = await self.client.post('/v1/auth/steam/start', json={'challenge':digest, 'confirmation':digest})
                await response.read()
            accepted += response.status == 201
        self.assertLessEqual(accepted, 8)
        self.assertLessEqual(len(self.steam.pending), 9)
        self.assertEqual(self.steam.pending[device].status, 'awaiting_confirmation')
        with patch.object(self.directory, 'source', return_value='2001:db9::1'):
            response = await self.client.post('/v1/auth/steam/start', json={'challenge':digest, 'confirmation':digest})
            self.assertEqual(response.status, 201, await response.text())
        token = self.steam.pending[device].browser_token
        response = await self.client.post('/v1/auth/steam/confirm/'+device, data={'code':'A1B2C3D4E5F6'},
            headers={'Origin':self.settings.public_url, 'Cookie':'steam-confirm='+token})
        self.assertEqual(response.status, 200)
        response = await self.client.post('/v1/auth/steam/poll', json={'device':device, 'verifier':self.verifier})
        self.assertEqual((await response.json())['state'], 'signed_in')

    async def test_anonymous_starts_have_an_independent_global_burst(self):
        digest = base64.urlsafe_b64encode(hashlib.sha256(self.verifier.encode()).digest()).rstrip(b'=').decode()
        statuses = []
        for index in range(20):
            with patch.object(self.directory, 'source', return_value=f'2001:db8:{index:x}::1'):
                response = await self.client.post('/v1/auth/steam/start', json={'challenge':digest, 'confirmation':digest})
                await response.read()
                statuses.append(response.status)
        self.assertEqual(statuses, [201]*16 + [429]*4)
        self.assertEqual(len(self.steam.pending), 16)

    async def test_general_concurrency_is_bounded_and_released_after_errors(self):
        slots = self.directory.request_slots
        slots.limit = 2
        entered, release = asyncio.Event(), asyncio.Event()
        async def body(*args):
            if slots.active == 2:
                entered.set()
            await release.wait()
            return {}
        with patch.object(self.directory, 'body', side_effect=body):
            tasks = [asyncio.create_task(self.client.post('/v1/servers', json={})) for _ in range(2)]
            try:
                await asyncio.wait_for(entered.wait(), 2)
                response = await self.client.post('/v1/servers', json={})
                self.assertEqual(response.status, 503)
                self.assertEqual(slots.active, 2)
            finally:
                release.set()
                await asyncio.gather(*tasks)
        self.assertEqual(slots.active, 0)
        self.assertEqual((await self.client.get('/v1/health')).status, 200)

    async def test_upgraded_websockets_do_not_hold_http_request_slots(self):
        from dataclasses import replace
        from aiohttp import web
        from aiohttp.test_utils import TestClient, TestServer
        from multiplayer.master_server.service import create_app
        app = create_app(replace(self.settings, state=self.root/'websocket'))
        directory = next(iter(app.values()))
        directory.request_slots.limit = 1
        async def websocket(request):
            socket = web.WebSocketResponse()
            await socket.prepare(request)
            await socket.receive()
            await socket.close()
            return socket
        app.router.add_get('/probe-ws', websocket)
        client = TestClient(TestServer(app))
        await client.start_server()
        self.addAsyncCleanup(client.close)
        socket = await client.ws_connect('/probe-ws')
        try:
            self.assertEqual(directory.request_slots.active, 0)
            self.assertEqual((await client.get('/v1/health')).status, 200)
        finally:
            await socket.close()
        self.assertEqual(directory.request_slots.active, 0)
