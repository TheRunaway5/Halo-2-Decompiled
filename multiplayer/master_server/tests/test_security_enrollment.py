"""Key squatting, proof replay and enrollment audience regressions."""
import unittest
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from multiplayer.master_server.protocol import public_hex, signed
from multiplayer.master_server.service import Directory
from . import test_http as http


class EnrollmentTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = http.HttpTests.asyncSetUp

    def registry(self):
        return next(value for value in self.client.app.values() if isinstance(value, Directory)).store

    async def challenge(self, key, token="owner-one", ttl=100, route="/v1/credentials", **extra):
        body = dict(server_public_key=public_hex(key), ttl_seconds=ttl, **extra)
        response = await self.client.post(route + "/challenge", json=body,
                                          headers={"Authorization": "Bearer " + token})
        self.assertEqual(response.status, 200, await response.text())
        return body | {"proof": signed(key, await response.json())}

    async def submit(self, body, token="owner-one", route="/v1/credentials"):
        return await self.client.post(route, json=body, headers={"Authorization": "Bearer " + token})

    async def test_unsigned_and_wrong_key_cannot_squat_then_legitimate_key_enrolls(self):
        key = Ed25519PrivateKey.generate()
        unsigned = dict(server_public_key=public_hex(key))
        self.assertEqual((await self.submit(unsigned)).status, 400)
        body = await self.challenge(key)
        forged = body | {"proof": signed(Ed25519PrivateKey.generate(), body["proof"]["payload"])}
        self.assertEqual((await self.submit(forged)).status, 401)
        self.assertEqual(self.registry().db.execute("SELECT count(*) FROM credentials").fetchone()[0], 0)
        self.assertEqual((await self.submit(body)).status, 201)

    async def test_cross_owner_master_lifetime_and_payload_changes_are_rejected(self):
        key = Ed25519PrivateKey.generate()
        body = await self.challenge(key)
        self.assertEqual((await self.submit(body, "owner-two")).status, 401)
        self.assertEqual((await self.submit(body | {"ttl_seconds": 101})).status, 401)
        payload = body["proof"]["payload"]
        altered = body | {"proof": signed(key, payload | {"master_id": "other-master"})}
        self.assertEqual((await self.submit(altered)).status, 401)
        self.assertEqual((await self.submit(body)).status, 201)
        self.assertEqual((await self.submit(body)).status, 401)

    async def test_expired_challenges_cannot_be_restored_by_clock_rollback(self):
        store = self.registry()
        now = store.now()
        store.clock = lambda: now
        body = await self.challenge(Ed25519PrivateKey.generate())
        store.clock = lambda: now + 121
        self.assertEqual((await self.submit(body)).status, 401)
        store.clock = lambda: now
        self.assertEqual((await self.submit(body)).status, 401)

    async def test_pending_challenges_are_bounded_per_owner_and_expire(self):
        store = self.registry()
        now = store.now()
        store.clock = lambda: now
        key = Ed25519PrivateKey.generate()
        for _ in range(8):
            await self.challenge(key)
        response = await self.client.post("/v1/credentials/challenge", json={"server_public_key": public_hex(key)},
                                          headers={"Authorization": "Bearer owner-one"})
        self.assertEqual(response.status, 429)
        await self.challenge(key, "owner-two")
        store.clock = lambda: now + 121
        await self.challenge(key)
        self.assertEqual(store.db.execute("SELECT count(*) FROM enrollment_challenges").fetchone()[0], 1)

    async def test_operator_also_proves_key_and_owner_label(self):
        directory = next(value for value in self.client.app.values() if isinstance(value, Directory))
        directory.settings.admin_token = "test-operator-token"
        key = Ed25519PrivateKey.generate()
        body = await self.challenge(key, "test-operator-token", route="/v1/admin/credentials", owner_label="a")
        self.assertEqual((await self.submit(body | {"owner_label": "b"}, "test-operator-token",
                                            "/v1/admin/credentials")).status, 401)
        self.assertEqual((await self.submit(body, "test-operator-token", "/v1/admin/credentials")).status, 201)
