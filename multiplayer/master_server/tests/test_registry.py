import tempfile
import unittest
import uuid
from pathlib import Path

from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey

from multiplayer.master_server.protocol import (Problem, public_hex, master_id, server_id, signed, parse)
from multiplayer.master_server.store import Store

DATA = {"name": "Simulated server", "host": "203.0.113.10", "port": 2002, "map": "lockout", "gametype": "Slayer",
        "players": 1, "max_players": 16, "game_version": "simulator-v1", "visibility": "public", "server_type": "dedicated", "region": "US-EAST"}


def enroll(store, provider, subject, key, ttl):
    challenge = store.enrollment_challenge(provider, subject, public_hex(key), ttl)
    return store.enroll(provider, subject, public_hex(key), ttl, signed(key, challenge))


class RegistryTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.now = 1000
        self.master = Ed25519PrivateKey.generate()
        self.instance = Ed25519PrivateKey.generate()
        self.store = Store(Path(self.directory.name) / "a.sqlite", self.master, 10, lambda: self.now)
        self.addCleanup(self.store.db.close)
        self.credential = enroll(self.store, "discord", "123456789012345678", self.instance, 100)
        self.sequence = 0

    def announcement(self, method="POST", suffix="", data=None, **overrides):
        self.sequence += 1
        path = "/v1/servers" + ("/" + self.credential["server_id"] if suffix or method != "POST" else "") + suffix
        value = {"v": 1, "kind": "announcement", "master_id": self.store.id, "server_id": self.credential["server_id"],
                 "credential_id": self.credential["credential_id"], "sequence": self.sequence,
                 "issued_at": self.now, "method": method, "path": path, "metadata": DATA if data is None else data}
        value.update(overrides)
        return signed(self.instance, value), path

    def send(self, method="POST", suffix="", data=None, **overrides):
        value, path = self.announcement(method, suffix, data, **overrides)
        return self.store.announce(value, method, path)

    def problem(self, status, function):
        with self.assertRaises(Problem) as caught:
            function()
        self.assertEqual(status, caught.exception.status)

    def test_lifecycle_and_filters(self):
        initial = self.send()
        self.assertEqual(1, len(self.store.discover({"map": "lockout", "available_slots": "15"})))
        self.assertEqual([], self.store.discover({"gametype": "CTF"}))
        self.assertEqual([], self.store.discover({"available_slots": "16"}))
        self.now += 2
        updated = self.send("PATCH", data={"map": "ascension", "players": 3})
        self.assertEqual(initial["registered_at"], updated["registered_at"])
        self.assertGreater(updated["expires_at"], initial["expires_at"])
        self.send("POST", "/heartbeat", {})
        self.assertEqual(1, len(self.store.discover({"region": "US-EAST", "server_type": "dedicated", "game_version": "simulator-v1"})))
        self.send("DELETE", data={})
        self.assertEqual([], self.store.discover())
        self.send()
        self.now += 11
        self.store.cleanup()
        self.assertEqual([], self.store.discover())
        self.problem(404, lambda: self.send("POST", "/heartbeat", {}))
        self.send()
        self.assertEqual(1, len(self.store.discover()))

    def test_replay_forgery_binding_and_validation(self):
        value, path = self.announcement()
        forged = signed(Ed25519PrivateKey.generate(), value["payload"])
        self.problem(401, lambda: self.store.announce(forged, "POST", path))
        self.problem(401, lambda: self.store.announce(value, "PATCH", path))
        self.store.announce(value, "POST", path)
        self.problem(409, lambda: self.store.announce(value, "POST", path))
        self.problem(409, lambda: self.send())
        self.problem(401, lambda: self.send("POST", "/heartbeat", {}, issued_at=self.now - 31))
        self.problem(401, lambda: self.send("POST", "/heartbeat", {}, master_id=str(uuid.uuid4())))
        self.problem(403, lambda: self.send("POST", "/heartbeat", {}, server_id=str(uuid.uuid4())))
        for patch in ({"players": 17}, {"max_players": 0}, {"port": True}, {"name": "bad\nname"}, {"extra": 1}, {"host": "http://evil/path"}):
            with self.subTest(patch=patch):
                self.problem(400, lambda: self.send("PATCH", data=patch))
        self.problem(400, lambda: parse('{"a":1,"a":2}'))
        self.problem(400, lambda: parse('{"a":NaN}'))

    def test_owner_revocation_and_credential_expiry(self):
        self.send()
        self.problem(403, lambda: self.store.revoke(self.credential["credential_id"], "discord", "999999999999999999"))
        self.store.revoke(self.credential["credential_id"], "discord", "123456789012345678")
        self.assertEqual([], self.store.discover())
        self.problem(403, lambda: self.send("POST", "/heartbeat", {}))
        key = Ed25519PrivateKey.generate()
        credential = enroll(self.store, "operator", "test", key, 1)
        self.now += 2
        envelope, path = self.announcement(credential_id=credential["credential_id"], server_id=credential["server_id"])
        envelope = signed(key, envelope["payload"])
        self.problem(403, lambda: self.store.announce(envelope, "POST", path))
        self.problem(409, lambda: enroll(self.store, "operator", "other", key, 100))

    def test_federation_expiry_tombstones_and_restart(self):
        other_key = Ed25519PrivateKey.generate()
        other_path = Path(self.directory.name) / "b.sqlite"
        other = Store(other_path, other_key, 10, lambda: self.now)
        trusted = {self.store.id: public_hex(self.master)}
        self.send()
        stale = self.store.snapshot()
        other.merge(stale, trusted)
        self.assertEqual(self.store.discover(), other.discover())
        self.now += 5
        other.merge(stale, trusted)
        self.assertEqual(1010, other.discover()[0]["expires_at"])
        self.now += 6
        other.merge(stale, trusted)
        self.assertEqual([], other.discover())
        self.now += 1
        self.send()
        other.merge(self.store.snapshot(), trusted)
        self.store.revoke(self.credential["credential_id"])
        other.merge(self.store.snapshot(), trusted)
        other.merge(stale, trusted)
        self.assertEqual([], other.discover())
        other.db.close()
        other = Store(other_path, other_key, 10, lambda: self.now)
        self.addCleanup(other.db.close)
        other.merge(stale, trusted)
        self.assertEqual([], other.discover())
        self.now = 999  # Observed expiry cannot be undone by a wall-clock rollback.
        self.assertEqual([], other.discover())

    def test_federation_origin_signatures_and_equivocation(self):
        other_key = Ed25519PrivateKey.generate()
        other = Store(Path(self.directory.name) / "b.sqlite", other_key, 10, lambda: self.now)
        self.addCleanup(other.db.close)
        self.send()
        snapshot = self.store.snapshot()
        self.problem(403, lambda: other.merge(snapshot, {}))
        trusted = {self.store.id: public_hex(self.master)}
        bad = signed(other_key, snapshot[0]["payload"])
        self.problem(401, lambda: other.merge([bad], trusted))
        other.merge(snapshot, trusted)
        changed = dict(snapshot[0]["payload"], metadata=DATA | {"name": "Equivocation"})
        self.problem(409, lambda: other.merge([signed(self.master, changed)], trusted))
        future = dict(snapshot[0]["payload"], revision=2, last_heartbeat=self.now + 31)
        self.problem(400, lambda: other.merge([signed(self.master, future)], trusted))
        too_long = dict(snapshot[0]["payload"], revision=2, expires_at=self.now + 181)
        self.problem(400, lambda: other.merge([signed(self.master, too_long)], trusted))

    def test_private_servers_and_opaque_owners(self):
        self.send(data=DATA | {"visibility": "private"})
        self.assertEqual([], self.store.discover())
        self.send("PATCH", data={"visibility": "public"})
        listing = self.store.discover()[0]
        self.assertNotIn("123456789012345678", str(listing))
        self.assertNotIn("discord", str(listing))
        self.assertEqual("unverified", listing["reachability"])

    def test_nonce_replay_and_bounded_retention(self):
        nonce = str(uuid.uuid4())
        self.store.nonce("peer", nonce)
        self.problem(409, lambda: self.store.nonce("peer", nonce))
        self.now += 62
        self.store.cleanup()
        self.store.nonce("peer", nonce)
