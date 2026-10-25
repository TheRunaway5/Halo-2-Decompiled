"""Regressions for the October 23 Security Cloud scan's backend boundaries."""
import asyncio
import sqlite3
import uuid
import unittest
from concurrent.futures import ThreadPoolExecutor
from threading import Barrier
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from multiplayer.master_server.protocol import Problem, public_hex, master_id, server_id, signed
from . import test_registry as registry
from . import test_steam_login as steam
from . import test_eos_verification_queue as eos


class FederationSecurityTests(unittest.TestCase):
    setUp = registry.RegistryTests.setUp
    announcement = registry.RegistryTests.announcement
    send = registry.RegistryTests.send

    def record(self, key, server=None):
        public = public_hex(server or Ed25519PrivateKey.generate())
        return signed(key, dict(v=1, kind='listing', origin_master_id=master_id(public_hex(key)),
            server_id=server_id(public), server_public_key=public, owner_id=str(uuid.uuid4()),
            credential_id=str(uuid.uuid4()), revision=1, registered_at=self.now,
            last_heartbeat=self.now, expires_at=self.now+10, state='active', metadata=registry.DATA))

    def test_one_origin_cannot_consume_local_or_other_origin_capacity(self):
        attacker, other = Ed25519PrivateKey.generate(), Ed25519PrivateKey.generate()
        trusted = {master_id(public_hex(k)): public_hex(k) for k in (attacker, other)}
        accepted = 0
        for _ in range(1000):
            try:
                self.store.merge([self.record(attacker)], trusted)
                accepted += 1
            except Problem:
                break
        self.assertLessEqual(accepted, 128)
        self.store.merge([self.record(other)], trusted)
        self.send()
        self.assertTrue(any(r['origin_master_id'] == self.store.id for r in self.store.discover()))

    def test_copied_public_identity_cannot_replace_a_different_origin(self):
        self.send()
        attacker = Ed25519PrivateKey.generate()
        trusted = {master_id(public_hex(attacker)): public_hex(attacker)}
        self.store.merge([self.record(attacker, self.instance)], trusted)
        rows = self.store.discover()
        self.assertEqual(len(rows), 2)
        self.assertEqual({r['origin_master_id'] for r in rows}, {self.store.id, *trusted})

    def test_expiry_and_unpin_reclaim_remote_rows_without_resurrection(self):
        key = Ed25519PrivateKey.generate(); origin = master_id(public_hex(key))
        record = self.record(key); trusted = {origin: public_hex(key)}
        self.store.merge([record], trusted)
        self.now += 300
        self.store.cleanup(trusted)
        self.assertEqual(self.store.db.execute('SELECT count(*) FROM records').fetchone()[0], 0)
        self.store.merge([record], trusted)
        self.assertEqual(self.store.discover(), [])
        fresh = self.record(key)
        self.store.merge([fresh], trusted)
        self.store.cleanup({})
        self.assertEqual(self.store.db.execute('SELECT count(*) FROM records').fetchone()[0], 0)

    def test_legacy_full_bank_is_quarantined_until_old_leases_cannot_replay(self):
        key = Ed25519PrivateKey.generate(); origin = master_id(public_hex(key))
        records = [self.record(key) for _ in range(1000)]
        with self.store.db:
            for row in records:
                self.store._put(row)
        self.store.cleanup({origin: public_hex(key)})
        self.send()
        self.assertEqual(len(self.store.discover()), 1)
        with self.assertRaises(Problem):
            self.store.merge([records[0]], {origin: public_hex(key)})
        self.now += 211
        self.store.merge([records[0]], {origin: public_hex(key)})
        self.assertFalse(any(r['origin_master_id']==origin for r in self.store.discover()))


class LinkedPlayerCapacityTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = steam.SteamLoginTests.asyncSetUp
    signature = steam.SteamLoginTests.signature
    start = steam.SteamLoginTests.start
    assertion = steam.SteamLoginTests.assertion
    complete = steam.SteamLoginTests.complete

    async def test_link_at_capacity_leaves_no_partial_identity_or_profile(self):
        _, result = await self.complete()
        db = self.steam.db
        with db:
            db.executemany('INSERT INTO players VALUES(?,?,?)',
                [(str(uuid.uuid4()), 'fixture', str(i)) for i in range(1000)])
        before = [db.execute('SELECT count(*) FROM '+t).fetchone()[0]
                  for t in ('players', 'player_profiles', 'player_identities')]
        response = await self.client.post('/v1/account/link-discord',
            headers={'Authorization': 'Bearer '+result['token']}, json={'discord_token': 'legacy'})
        self.assertEqual(response.status, 503, await response.text())
        self.assertEqual(before, [db.execute('SELECT count(*) FROM '+t).fetchone()[0]
                         for t in ('players', 'player_profiles', 'player_identities')])


class PlayerAllocatorConcurrencyTests(unittest.TestCase):
    setUp = registry.RegistryTests.setUp

    def test_two_connections_cannot_both_allocate_the_final_player_slot(self):
        from multiplayer.master_server.player_identity import initialize, allocate
        db = self.store.db; initialize(db)
        with db:
            db.executemany('INSERT INTO players VALUES(?,?,?)',
                [(str(uuid.uuid4()), 'fixture', str(i)) for i in range(999)])
        path = db.execute('PRAGMA database_list').fetchone()[2]
        barrier = Barrier(2)
        def insert(index):
            connection = sqlite3.connect(path)
            try:
                barrier.wait(timeout=5)
                try:
                    with connection:
                        allocate(connection, ('steam', str(index)))
                    return 201
                except Problem as error:
                    return error.status
            finally:
                connection.close()
        with ThreadPoolExecutor(max_workers=2) as pool:
            self.assertEqual(sorted(pool.map(insert, range(2))), [201, 503])
        self.assertEqual(db.execute('SELECT count(*) FROM players').fetchone()[0], 1000)
        self.assertEqual(db.execute('SELECT count(*) FROM player_identities').fetchone()[0], 1)


class EosPrincipalCapacityTests(eos.EosVerificationQueueTests):
    async def test_one_authenticated_principal_cannot_queue_multiple_puids(self):
        identity = self.configure(); identity.binary = 'test-worker'
        worker = eos.Worker(); identity.process = worker
        budgets, _ = self.budget()
        with budgets:
            active = await self.task(identity.verify('1'*32, 'first', principal='attacker'))
            await asyncio.wait_for(worker.written.wait(), 1)
            for i in range(32):
                with self.assertRaises(Problem) as failure:
                    await identity.verify(f'{i+2:032x}', 'flood', principal='attacker')
                self.assertEqual(failure.exception.status, 429)
            other = await self.task(identity.verify('f'*32, 'other', principal='other'))
            await asyncio.sleep(0)
            self.assertEqual(identity.pending, 2)
            worker.reply('attacker'); self.assertEqual(await active, 'attacker')
            worker.reply('other'); self.assertEqual(await other, 'other')
        self.assertEqual(identity.pending, 0)
