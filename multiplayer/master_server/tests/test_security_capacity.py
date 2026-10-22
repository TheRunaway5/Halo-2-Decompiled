"""Shared capacity must remain available when one authenticated owner churns."""
import hashlib
import unittest
from concurrent.futures import ThreadPoolExecutor
from threading import Barrier
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from multiplayer.master_server.protocol import Problem, public_hex
from multiplayer.master_server.store import Store
from . import test_registry as registry
from . import test_steam_login as steam


class CredentialCapacityTests(unittest.TestCase):
    setUp = registry.RegistryTests.setUp

    def test_one_owner_is_limited_before_it_can_fill_the_global_bank(self):
        accepted = 1  # setUp already enrolled this owner.
        for _ in range(1000):
            try:
                registry.enroll(self.store, 'discord', '123456789012345678', Ed25519PrivateKey.generate(), 100)
                accepted += 1
            except Problem as error:
                self.assertEqual(error.status, 429)
                break
        self.assertLessEqual(accepted, 8)
        registry.enroll(self.store, 'discord', 'other-owner', Ed25519PrivateKey.generate(), 100)

    def test_revoked_and_expired_credentials_do_not_consume_live_capacity(self):
        # A historical full bank must recover without deleting key/replay history.
        with self.store.db:
            for i in range(999):
                self.store.db.execute('INSERT INTO credentials VALUES(?,?,?,?,?,?,?)',
                    (str(i), str(i), self.credential['owner_id'], public_hex(Ed25519PrivateKey.generate()),
                     self.now - 1 if i % 2 else self.now + 100, int(i % 2 == 0), 0))
        registry.enroll(self.store, 'discord', 'other-owner', Ed25519PrivateKey.generate(), 100)
        self.assertEqual(self.store.db.execute('SELECT count(*) FROM credentials').fetchone()[0], 1001)

    def test_concurrent_registries_cannot_race_the_last_owner_slot(self):
        for _ in range(6):
            registry.enroll(self.store, 'discord', '123456789012345678', Ed25519PrivateKey.generate(), 100)
        barrier = Barrier(2)
        # SQLite connections belong to their worker thread, including lookup.
        path = self.store.db.execute('PRAGMA database_list').fetchone()[2]
        def worker(_):
            store = Store(path, self.master, clock=lambda: self.now)
            try:
                barrier.wait(timeout=5)
                try:
                    registry.enroll(store, 'discord', '123456789012345678', Ed25519PrivateKey.generate(), 100)
                    return 201
                except Problem as error:
                    return error.status
            finally:
                store.db.close()
        with ThreadPoolExecutor(max_workers=2) as pool:
            self.assertEqual(sorted(pool.map(worker, range(2))), [201, 429])
        self.assertEqual(self.store.db.execute('SELECT count(*) FROM credentials').fetchone()[0], 8)

    def test_repeated_revocation_is_bounded_by_the_owner_key_history(self):
        self.store.revoke(self.credential['credential_id'])
        for _ in range(63):
            credential = registry.enroll(self.store, 'discord', '123456789012345678', Ed25519PrivateKey.generate(), 100)
            self.store.revoke(credential['credential_id'])
        with self.assertRaises(Problem) as error:
            registry.enroll(self.store, 'discord', '123456789012345678', Ed25519PrivateKey.generate(), 100)
        self.assertEqual(error.exception.status, 429)
        registry.enroll(self.store, 'discord', 'other-owner', Ed25519PrivateKey.generate(), 100)


class SteamCapacityTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = steam.SteamLoginTests.asyncSetUp
    signature = steam.SteamLoginTests.signature
    start = steam.SteamLoginTests.start
    assertion = steam.SteamLoginTests.assertion
    complete = steam.SteamLoginTests.complete

    async def test_one_subject_rotates_sessions_and_another_subject_can_sign_in(self):
        subject = '76561197960287930'
        with self.steam.db:
            self.steam.db.executemany('INSERT INTO steam_login_sessions VALUES(?,?,?,?)',
                [(hashlib.sha256(str(i).encode()).hexdigest(), subject, 'Fixture', int(self.now) + 28800)
                 for i in range(999)])
        _, first = await self.complete(subject)
        count = self.steam.db.execute('SELECT count(*) FROM steam_login_sessions WHERE subject=?', (subject,)).fetchone()[0]
        self.assertLessEqual(count, 4)
        _, other = await self.complete('76561197960287931')
        self.assertEqual(other['state'], 'signed_in')
        self.assertEqual((await self.steam.owner(first['token'])).subject, subject)

    async def test_repeated_login_retires_old_tokens_and_retains_newest_four(self):
        tokens = []
        for _ in range(6):
            _, result = await self.complete()
            tokens.append(result['token'])
        self.assertEqual(self.steam.db.execute('SELECT count(*) FROM steam_login_sessions').fetchone()[0], 4)
        for token in tokens[:2]:
            with self.assertRaises(Problem) as error:
                await self.steam.owner(token)
            self.assertEqual(error.exception.status, 401)
        for token in tokens[2:]:
            self.assertEqual((await self.steam.owner(token)).subject, '76561197960287930')

