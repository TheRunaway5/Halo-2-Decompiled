"""Resource amplification regressions for collection discovery and federation."""
import asyncio
import sqlite3
import time
import unittest
import uuid
from urllib.parse import parse_qs, urlsplit
from unittest.mock import AsyncMock, patch

from multiplayer.master_server.protocol import MAX_SYNC, Problem, canonical, metadata, signed, verified, public_hex
from multiplayer.master_server.service import Directory
from . import test_http as http
from .test_registry import DATA


class DirectoryResourceTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = http.HttpTests.asyncSetUp

    @property
    def service(self):
        return next(value for value in self.client.server.app.values() if isinstance(value, Directory))

    def populate(self, count=1000):
        # Maximum length UTF-8 text also exercises JSON escaping expansion.
        data = DATA | {name: "\u0080" * 32 for name in ("name", "map", "gametype", "game_version")}
        data["host"] = ".".join(["h" * 63] * 3 + ["h" * 61])
        metadata(data)
        now = int(time.time())
        with self.service.store.db:
            for index in range(count):
                payload = dict(v=1, kind="listing", origin_master_id=self.service.id,
                    server_id=str(uuid.UUID(int=index+1)), server_public_key="00"*32,
                    owner_id=str(uuid.UUID(int=1)), credential_id=str(uuid.UUID(int=2)),
                    revision=1, registered_at=now, last_heartbeat=now, expires_at=now+180,
                    state="active", metadata=data)
                self.service.store._put(signed(self.service.key, payload))

    async def test_populated_public_response_has_page_and_byte_limits(self):
        self.populate()
        response = await self.client.get("/v1/servers")
        raw = await response.read()
        self.assertEqual(response.status, 200)
        page = await response.json()
        self.assertLessEqual(len(page["servers"]), 64)
        self.assertLessEqual(len(raw), 64*1024)
        self.assertTrue(page.get("next_cursor"))

    async def test_fresh_signed_nonces_do_not_allow_a_sync_burst(self):
        statuses = []
        for _ in range(6):
            payload = dict(v=1, kind="sync", sender_id=self.peer_id, recipient_id=self.service.id,
                           issued_at=int(time.time()), nonce=str(uuid.uuid4()))
            response = await self.client.post("/v1/federation/sync", json=signed(self.peer_key, payload))
            await response.read()
            statuses.append(response.status)
        self.assertEqual(statuses, [200, 429, 429, 429, 429, 429])
        key = "peer:" + self.peer_id
        tokens, previous = self.service.buckets[key]
        self.service.buckets[key] = tokens, previous - self.service.settings.sync_interval
        payload["nonce"] = str(uuid.uuid4())
        response = await self.client.post("/v1/federation/sync", json=signed(self.peer_key, payload))
        self.assertEqual(response.status, 200)
        reply = verified(await response.json(), public_hex(self.service.key))
        self.assertEqual(reply["nonce"], payload["nonce"])

    async def test_all_pages_are_complete_and_bounded(self):
        self.populate()
        cursor, found = None, []
        for _ in range(32):
            response = await self.client.get("/v1/servers", params={"cursor": cursor} if cursor else {})
            self.assertEqual(response.status, 200, await response.text())
            self.assertLessEqual(len(await response.read()), 64*1024)
            page = await response.json()
            self.assertLessEqual(len(page["servers"]), 64)
            found.extend((v["origin_master_id"], v["server_id"]) for v in page["servers"])
            previous, cursor = cursor, page.get("next_cursor")
            if cursor is None:
                break
            self.assertTrue(previous is None or cursor > previous)
        self.assertIsNone(cursor)
        self.assertEqual(len(found), 1000)
        self.assertEqual(found, sorted(set(found)))

    async def test_bad_pagination_and_private_expired_filtered_records(self):
        self.populate(5)
        store = self.service.store
        with store.db:
            records = store.snapshot()
            for index, envelope in enumerate(records[:3]):
                payload = envelope["payload"]
                if index == 0:
                    payload["metadata"]["visibility"] = "private"
                elif index == 1:
                    payload["expires_at"] = int(time.time()) - 1
                else:
                    payload["state"] = "deleted"
                store._put(signed(self.service.key, payload))
        response = await self.client.get("/v1/servers?limit=1&available_slots=15")
        page = await response.json()
        self.assertEqual(len(page["servers"]), 1)
        self.assertTrue(page["next_cursor"])
        response = await self.client.get("/v1/servers", params={"limit": 1, "cursor": page["next_cursor"], "available_slots": 15})
        self.assertEqual(len((await response.json())["servers"]), 1)
        response = await self.client.get("/v1/servers?region=EU")
        self.assertEqual((await response.json())["servers"], [])
        for query in ("limit=0", "limit=65", "limit=-1", "limit=one", "cursor=oops", "cursor=", "limit=1&limit=2"):
            response = await self.client.get("/v1/servers?" + query)
            self.assertEqual(response.status, 400, query)
        response = await self.client.get("/v1/servers/" + records[0]["payload"]["server_id"])
        self.assertEqual(response.status, 404)

    async def test_source_burst_is_small_and_health_remains_responsive(self):
        statuses = []
        for _ in range(40):
            response = await self.client.get("/v1/servers")
            await response.read()
            statuses.append(response.status)
        self.assertEqual(statuses[:32], [200]*32)
        self.assertEqual(statuses[32:], [429]*8)
        self.assertEqual((await self.client.get("/v1/health")).status, 200)

    async def test_slow_discovery_holds_global_admission_until_drain(self):
        budget = self.service.responses
        entered, release = asyncio.Event(), asyncio.Event()
        original = budget.send
        async def slow(*args):
            if budget.active == budget.request_limit:
                entered.set()
            await release.wait()
            return await original(*args)
        with patch.object(budget, "send", side_effect=slow):
            tasks = [asyncio.create_task(self.client.get("/v1/servers")) for _ in range(4)]
            try:
                await asyncio.wait_for(entered.wait(), 2)
                self.assertEqual(budget.reserved_bytes, 4*64*1024)
                response = await self.client.get("/v1/servers")
                self.assertEqual(response.status, 503)
                self.assertEqual((await self.client.get("/v1/health")).status, 200)
            finally:
                release.set()
                await asyncio.gather(*tasks)
        self.assertEqual((budget.active, budget.reserved_bytes), (0, 0))

    async def test_peer_single_flight_and_byte_budget_precede_materialization(self):
        entered, release = asyncio.Event(), asyncio.Event()
        budget = self.service.responses
        original = budget.send
        async def slow(*args):
            entered.set()
            await release.wait()
            return await original(*args)
        def envelope():
            return signed(self.peer_key, dict(v=1, kind="sync", sender_id=self.peer_id,
                recipient_id=self.service.id, issued_at=int(time.time()), nonce=str(uuid.uuid4())))
        with patch.object(budget, "send", side_effect=slow):
            task = asyncio.create_task(self.client.post("/v1/federation/sync", json=envelope()))
            try:
                await asyncio.wait_for(entered.wait(), 2)
                self.assertEqual(budget.reserved_bytes, MAX_SYNC)
                with patch.object(self.service.store, "snapshot_bytes", side_effect=AssertionError("Must reject before snapshot")):
                    response = await self.client.post("/v1/federation/sync", json=envelope())
                    self.assertEqual(response.status, 429)
                    with budget.reserve(MAX_SYNC):
                        response = await self.client.post("/v1/federation/sync", json=envelope())
                        self.assertEqual(response.status, 503)
            finally:
                release.set()
                response = await task
                payload = verified(await response.json(), public_hex(self.service.key))
                self.assertEqual(payload["recipient_id"], self.peer_id)
        self.assertFalse(budget.peers)
        self.assertEqual((budget.active, budget.reserved_bytes), (0, 0))

    async def test_cache_changes_only_with_record_revision_and_rechecks_expiry(self):
        self.populate(3)
        store = self.service.store
        from multiplayer.master_server import store as module
        with patch.object(module, "parse", wraps=module.parse) as parse:
            first = store.snapshot_bytes()
            self.assertEqual(parse.call_count, 3)
            store.discover()
            store.nonce(self.peer_id, str(uuid.uuid4()))
            self.assertIs(store.snapshot_bytes(), first)
            store.cleanup(self.service.trusted)
            self.assertIs(store.snapshot_bytes(), first)
            self.assertEqual(parse.call_count, 3)
            with patch.object(store, "clock", return_value=int(time.time())+181):
                self.assertEqual(store.discover(), [])
                self.assertIs(store.snapshot_bytes(), first)
            with store.db:
                store.db.execute("DELETE FROM records WHERE server=?", (str(uuid.UUID(int=1)),))
            self.assertNotEqual(store.snapshot_bytes(), first)
            self.assertEqual(parse.call_count, 5)

    async def test_another_connection_invalidates_cache_and_rollback_does_not(self):
        self.populate(3)
        store = self.service.store
        first = store.snapshot_bytes()
        connection = sqlite3.connect(self.directory.name + "/registry.sqlite3")
        self.addCleanup(connection.close)
        with connection:
            connection.execute("DELETE FROM records WHERE server=?", (str(uuid.UUID(int=1)),))
        second = store.snapshot_bytes()
        self.assertNotEqual(second, first)
        self.assertEqual(len(store.snapshot()), 2)
        try:
            with connection:
                connection.execute("DELETE FROM records")
                raise RuntimeError("rollback")
        except RuntimeError:
            pass
        self.assertIs(store.snapshot_bytes(), second)
        with connection:
            connection.execute("UPDATE records SET envelope=envelope")
        self.assertIs(store.snapshot_bytes(), second)

    async def test_response_write_timeout_and_cancellation_release_capacity(self):
        budget = self.service.responses
        request = unittest.mock.Mock()
        request.transport = unittest.mock.Mock()
        for error in (TimeoutError(), asyncio.CancelledError(), ConnectionResetError()):
            with self.subTest(error=type(error).__name__):
                with patch("multiplayer.master_server.directory_resources.web.Response.prepare", new=AsyncMock(side_effect=error)):
                    with self.assertRaises(type(error)):
                        with budget.reserve(MAX_SYNC), budget.peer(self.peer_id):
                            await budget.send(request, b"{}")
                request.transport.abort.assert_called()
                self.assertEqual((budget.active, budget.reserved_bytes), (0, 0))
                self.assertFalse(budget.peers)
        real_timeout = asyncio.timeout
        async def blocked(*args):
            await asyncio.Event().wait()
        with patch("multiplayer.master_server.directory_resources.web.Response.prepare", new=blocked), \
             patch("multiplayer.master_server.directory_resources.asyncio.timeout", side_effect=lambda _: real_timeout(0.01)):
            with self.assertRaises(TimeoutError):
                with budget.reserve(MAX_SYNC), budget.peer(self.peer_id):
                    await budget.send(request, b"{}")
        self.assertEqual((budget.active, budget.reserved_bytes), (0, 0))
        self.assertFalse(budget.peers)


class DiscoveryClientTests(unittest.TestCase):
    def test_cli_follows_pages_and_keeps_filters(self):
        from multiplayer.master_server.cli import discover
        cursor = str(uuid.UUID(int=1)) + "." + str(uuid.UUID(int=2))
        with patch("multiplayer.master_server.cli.request", side_effect=[
                {"servers": [{"name": "one"}], "next_cursor": cursor},
                {"servers": [{"name": "two"}]}]) as request:
            result = discover(["https://master.example"], {"map": "lockout"})
        self.assertEqual([row["name"] for row in result["servers"]], ["one", "two"])
        self.assertEqual(request.call_count, 2)
        path = urlsplit(request.call_args.args[2])
        self.assertEqual(path.path, "/v1/servers")
        self.assertEqual(parse_qs(path.query), {"map": ["lockout"], "cursor": [cursor]})

    def test_cli_rejects_unsafe_and_looping_continuations(self):
        from multiplayer.master_server.cli import discover
        cursor = str(uuid.UUID(int=1)) + "." + str(uuid.UUID(int=2))
        for token in ("https://evil.example/", cursor):
            with self.subTest(token=token):
                with patch("multiplayer.master_server.cli.request", return_value={"servers": [{}], "next_cursor": token}) as request:
                    with self.assertRaises(RuntimeError):
                        discover(["https://master.example"])
                self.assertLessEqual(request.call_count, 2)
