"""Directory lifecycle regressions from the October 25 Security Cloud scan."""
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import Mock, patch

from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey

from multiplayer.master_server.protocol import MAX_LEASE, SKEW, Problem, public_hex, signed
from multiplayer.master_server.store import Store
from . import test_registry as registry
from . import test_security_scan_20261023 as previous


class SecurityLibraryAbiTests(unittest.TestCase):
    def test_stale_service_library_fails_closed_before_new_symbol_lookup(self):
        from multiplayer.master_server import security_policy
        legacy = SimpleNamespace(h2_security_abi=Mock(return_value=1))
        security_policy.library.cache_clear()
        self.addCleanup(security_policy.library.cache_clear)
        with patch.dict('os.environ', H2_SECURITY_LIBRARY=str(Path(__file__).resolve())), \
                patch.object(security_policy.ctypes, 'CDLL', return_value=legacy):
            with self.assertRaises(Problem) as error:
                security_policy.library()
        self.assertEqual(error.exception.status, 503)
        self.assertEqual(error.exception.message, 'Rust security library ABI mismatch')


class LocalListingLifecycleTests(unittest.TestCase):
    setUp = registry.RegistryTests.setUp
    send = registry.RegistryTests.send
    announcement = registry.RegistryTests.announcement

    def announce(self, credential, key, sequence=1):
        return self.store.announce(signed(key, dict(v=1, kind='announcement', master_id=self.store.id,
            server_id=credential['server_id'], credential_id=credential['credential_id'], sequence=sequence,
            issued_at=self.now, method='POST', path='/v1/servers', metadata=registry.DATA)),
            'POST', '/v1/servers')

    def test_revoking_unused_credential_never_allocates_listing_history(self):
        self.store.revoke(self.credential['credential_id'])
        self.store.revoke(self.credential['credential_id'])
        self.assertEqual(self.store.snapshot(), [])
        with self.assertRaises(Problem) as error:
            self.send()
        self.assertEqual(error.exception.status, 403)

    def test_1024_unused_revocations_leave_local_and_foreign_admission_available(self):
        for owner in range(16):
            for _ in range(64):
                key = Ed25519PrivateKey.generate()
                credential = registry.enroll(self.store, 'discord', f'cycling-owner-{owner}', key, 100)
                self.store.revoke(credential['credential_id'])
        self.store.cleanup()
        self.assertEqual(self.store.snapshot(), [])
        self.send()
        other = Ed25519PrivateKey.generate()
        foreign = previous.FederationSecurityTests.record(self, other)
        self.store.merge([foreign], {foreign['payload']['origin_master_id']: public_hex(other)})
        self.assertEqual(len(self.store.discover()), 2)

    def test_announced_churn_has_an_owner_quota_and_terminal_rows_are_reclaimed(self):
        for _ in range(16):
            key = Ed25519PrivateKey.generate()
            credential = registry.enroll(self.store, 'discord', 'cycling-owner', key, 10000)
            self.announce(credential, key)
            self.store.revoke(credential['credential_id'])
        key = Ed25519PrivateKey.generate()
        credential = registry.enroll(self.store, 'discord', 'cycling-owner', key, 10000)
        with self.assertRaises(Problem) as error:
            self.announce(credential, key)
        self.assertEqual(error.exception.status, 429)
        self.send()
        self.now += MAX_LEASE + SKEW + self.store.lease + 1
        self.store.cleanup()
        self.assertEqual(self.store.snapshot(), [])
        self.announce(credential, key)
        self.assertEqual(len(self.store.discover()), 1)

    def test_local_partition_reserves_foreign_capacity_and_full_registry_allows_revocation(self):
        self.send()
        for index in range(487):
            key = Ed25519PrivateKey.generate()
            credential = registry.enroll(self.store, 'discord', f'local-owner-{index//16}', key, 10000)
            self.announce(credential, key)
            self.store.revoke(credential['credential_id'])
        key = Ed25519PrivateKey.generate()
        credential = registry.enroll(self.store, 'discord', 'unrelated-owner', key, 10000)
        with self.assertRaises(Problem) as error:
            self.announce(credential, key)
        self.assertEqual(error.exception.status, 503)
        peers = [Ed25519PrivateKey.generate() for _ in range(4)]
        records = [[previous.FederationSecurityTests.record(self, peer) for _ in range(128)] for peer in peers]
        trusted = {bank[0]['payload']['origin_master_id']: public_hex(peer) for bank, peer in zip(records, peers)}
        for bank in records:
            self.store.merge(bank, trusted)
        self.assertEqual(len(self.store.snapshot()), 1000)
        self.store.revoke(credential['credential_id'])  # Unused, even at capacity.
        self.store.revoke(self.credential['credential_id'])  # Existing published lease.
        self.assertEqual(len(self.store.snapshot()), 1000)
        self.assertEqual(len(self.store.discover()), 512)
        self.now += MAX_LEASE + SKEW + self.store.lease + 1
        self.store.cleanup(trusted)
        self.assertEqual(self.store.snapshot(), [])
        self.assertEqual(self.store.db.execute('SELECT count(*) FROM local_listing_revisions').fetchone()[0], 488)

    def test_reclaimed_live_credential_keeps_revision_watermark_across_restart(self):
        with self.store.db:
            self.store.db.execute('UPDATE credentials SET expires=? WHERE id=?',
                                  (self.now + 10000, self.credential['credential_id']))
        self.send()
        self.send('DELETE', data={})
        old = self.store.snapshot()
        self.now += MAX_LEASE + SKEW + self.store.lease + 1
        self.store.cleanup()
        self.assertEqual(self.store.snapshot(), [])
        path = Path(self.store.db.execute('PRAGMA database_list').fetchone()[2])
        reopened = Store(path, self.master, 10, lambda: self.now)
        self.addCleanup(reopened.db.close)
        previous, self.store = self.store, reopened
        self.assertEqual(self.send()['revision'], 3)
        receiver = Store(Path(self.directory.name) / 'receiver.sqlite', Ed25519PrivateKey.generate(), clock=lambda: self.now)
        self.addCleanup(receiver.db.close)
        trusted = {self.store.id: public_hex(self.master)}
        receiver.merge(self.store.snapshot(), trusted)
        receiver.merge(old, trusted)
        self.assertEqual(len(receiver.discover()), 1)
        self.store = previous

    def test_revoked_terminal_cleanup_preserves_credential_replay_rejection(self):
        envelope, path = self.announcement()
        self.store.announce(envelope, 'POST', path)
        self.store.revoke(self.credential['credential_id'])
        self.now += MAX_LEASE + SKEW + self.store.lease + 1
        self.store.cleanup()
        self.assertEqual(self.store.snapshot(), [])
        envelope['payload']['issued_at'] = self.now
        envelope['payload']['sequence'] += 1
        with self.assertRaises(Problem) as error:
            self.store.announce(signed(self.instance, envelope['payload']), 'POST', path)
        self.assertEqual(error.exception.status, 403)
        with self.assertRaises(Problem) as error:
            registry.enroll(self.store, 'discord', 'fresh-owner', self.instance, 100)
        self.assertEqual(error.exception.status, 409)

    def test_legacy_local_revision_is_migrated_before_reclamation(self):
        self.send()
        record = self.store.snapshot()[0]
        record['payload']['revision'] = 41
        with self.store.db:
            self.store._put(signed(self.master, record['payload']))
        path = self.store.db.execute('PRAGMA database_list').fetchone()[2]
        reopened = Store(path, self.master, 10, lambda: self.now)
        self.addCleanup(reopened.db.close)
        self.now += 11
        old, self.store = self.store, reopened
        self.assertEqual(self.send()['revision'], 42)
        self.store = old


if __name__ == '__main__':
    unittest.main()
