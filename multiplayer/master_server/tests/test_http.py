import asyncio
import datetime
import ipaddress
import ssl
import tempfile
import time
import unittest
import uuid
from pathlib import Path

from aiohttp import ClientSession, ClientConnectorCertificateError
from aiohttp.test_utils import TestClient, TestServer
from cryptography import x509
from cryptography.hazmat.primitives.serialization import Encoding, PrivateFormat, NoEncryption
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey

from multiplayer.master_server.protocol import Problem, public_hex, master_id, signed
from multiplayer.master_server.service import Settings, create_app, discord_owner, bounded_json


class HttpTests(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.peer_key = Ed25519PrivateKey.generate()
        self.peer_id = master_id(public_hex(self.peer_key))
        settings = Settings(state=Path(self.directory.name), public_url="http://127.0.0.1:8080", allow_http=True,
                            peers=[{"id": self.peer_id, "public_key": public_hex(self.peer_key), "url": "http://127.0.0.1:1"}])
        async def verifier(session, token, client):
            self.assertEqual("1558522073272426519", client)
            if token not in ("owner-one", "owner-two"):
                raise Problem(401, "Invalid test identity")
            return "discord", "123456789012345678" if token == "owner-one" else "999999999999999999"
        self.client = TestClient(TestServer(create_app(settings, verifier)))
        await self.client.start_server()
        self.addAsyncCleanup(self.client.close)

    async def test_owner_enrollment_revocation_and_privacy(self):
        key = Ed25519PrivateKey.generate()
        body = {"server_public_key": public_hex(key)}
        response = await self.client.post("/v1/credentials", json=body)
        self.assertEqual(401, response.status)
        challenge = await self.client.post("/v1/credentials/challenge", json=body,
                                           headers={"Authorization": "Bearer owner-one"})
        self.assertEqual(200, challenge.status)
        body["proof"] = signed(key, await challenge.json())
        response = await self.client.post("/v1/credentials", json=body, headers={"Authorization": "Bearer owner-one"})
        self.assertEqual(201, response.status)
        credential = await response.json()
        self.assertNotIn("123456789012345678", str(credential))
        response = await self.client.get("/v1/credentials", headers={"Authorization": "Bearer owner-two"})
        self.assertEqual([], (await response.json())["credentials"])
        path = "/v1/credentials/" + credential["credential_id"]
        response = await self.client.delete(path, headers={"Authorization": "Bearer owner-two"})
        self.assertEqual(403, response.status)
        response = await self.client.delete(path, headers={"Authorization": "Bearer owner-one"})
        self.assertEqual(200, response.status)

    async def test_federation_authentication_replay_and_audience(self):
        own = (await (await self.client.get("/v1/masters")).json())["master"]["id"]
        value = {"v": 1, "kind": "sync", "sender_id": self.peer_id, "recipient_id": own,
                 "issued_at": int(time.time()), "nonce": str(uuid.uuid4())}
        route = "/v1/federation/sync"
        response = await self.client.post(route, json=signed(Ed25519PrivateKey.generate(), value))
        self.assertEqual(401, response.status)
        response = await self.client.post(route, json=signed(self.peer_key, value | {"recipient_id": str(uuid.uuid4())}))
        self.assertEqual(401, response.status)
        response = await self.client.post(route, json=signed(self.peer_key, value))
        self.assertEqual(200, response.status)
        response = await self.client.post(route, json=signed(self.peer_key, value))
        self.assertEqual(409, response.status)
        response = await self.client.post(route, json=signed(self.peer_key, value | {"sender_id": str(uuid.uuid4())}))
        self.assertEqual(403, response.status)

    async def test_http_boundaries(self):
        response = await self.client.post("/v1/servers", data=b"x" * 17000, headers={"Content-Type": "application/json"})
        self.assertEqual(413, response.status)
        response = await self.client.post("/v1/servers", data='{"a":1,"a":2}', headers={"Content-Type": "application/json"})
        self.assertEqual(400, response.status)
        response = await self.client.post("/v1/servers", data=b"bad", headers={"Content-Type": "application/json", "Content-Encoding": "gzip"})
        self.assertEqual(415, response.status)
        for query in ("available_slots=oops", "unknown=value", "map=a&map=b"):
            response = await self.client.get("/v1/servers?" + query)
            self.assertEqual(400, response.status)

    async def test_rate_limit(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        client = TestClient(TestServer(create_app(Settings(state=Path(directory.name), allow_http=True,
                     public_url="http://127.0.0.1:8080", rate_per_minute=3))))
        await client.start_server()
        try:
            statuses = [(await client.get("/v1/health")).status for _ in range(5)]
            self.assertEqual([200, 200, 200, 429, 429], statuses)
        finally:
            await client.close()

    async def test_public_http_configuration_rejected(self):
        with self.assertRaises(Problem):
            Settings().validate()


class FakeContent:
    def __init__(self, value):
        import json
        self.data = json.dumps(value).encode()
    async def iter_chunked(self, size):
        yield self.data


class FakeResponse:
    status = 200
    content_length = None
    headers = {}
    def __init__(self, value): self.content = FakeContent(value)
    async def __aenter__(self): return self
    async def __aexit__(self, *args): pass


class DiscordVerificationTests(unittest.IsolatedAsyncioTestCase):
    async def test_upstream_compression_is_rejected_before_reading(self):
        response = FakeResponse({})
        response.headers = {"Content-Encoding": "gzip"}
        with self.assertRaises(Problem) as caught:
            await bounded_json(response)
        self.assertEqual(502, caught.exception.status)

    async def test_existing_token_validated_for_application_scope_and_user(self):
        calls = []
        class Session:
            def get(self, url, **kwargs):
                calls.append((url, kwargs))
                return FakeResponse({"application": {"id": "1558522073272426519"}, "scopes": ["identify"]}
                    if "oauth2" in url else {"id": "123456789012345678", "username": "test_pilot"})
        result = await discord_owner(Session(), "existing-access-token", "1558522073272426519")
        self.assertEqual(("discord", "123456789012345678", "test_pilot"), (result.provider, result.subject, result.username))
        self.assertEqual(2, len(calls))
        self.assertTrue(all(c[0].startswith("https://discord.com/api/v10/") and not c[1]["allow_redirects"] for c in calls))
        with self.assertRaises(Problem):
            await discord_owner(Session(), "existing-access-token", "999999999999999999")


class TlsTests(unittest.IsolatedAsyncioTestCase):
    async def test_direct_tls_requires_a_trusted_certificate(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder)
            key = Ed25519PrivateKey.generate()
            name = x509.Name([x509.NameAttribute(x509.oid.NameOID.COMMON_NAME, "localhost")])
            now = datetime.datetime.now(datetime.timezone.utc)
            cert = (x509.CertificateBuilder().subject_name(name).issuer_name(name)
                    .public_key(key.public_key()).serial_number(x509.random_serial_number())
                    .not_valid_before(now - datetime.timedelta(minutes=1))
                    .not_valid_after(now + datetime.timedelta(hours=1))
                    .add_extension(x509.SubjectAlternativeName([x509.IPAddress(ipaddress.ip_address("127.0.0.1"))]), critical=False)
                    .sign(key, None))
            cert_path, key_path = path / "tls.pem", path / "tls-key.pem"
            cert_path.write_bytes(cert.public_bytes(Encoding.PEM))
            key_path.write_bytes(key.private_bytes(Encoding.PEM, PrivateFormat.PKCS8, NoEncryption()))
            context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
            context.minimum_version = ssl.TLSVersion.TLSv1_2
            context.load_cert_chain(cert_path, key_path)
            server = TestServer(create_app(Settings(state=path / "state", cert=str(cert_path), tls_key=str(key_path))))
            await server.start_server(ssl=context)
            loop = asyncio.get_running_loop()
            previous_handler = loop.get_exception_handler()
            def handshake_error(loop, details):
                # Rejecting the self-signed peer deliberately resets a handshake.
                if isinstance(details.get("exception"), ConnectionResetError):
                    return
                (previous_handler or (lambda loop, details: loop.default_exception_handler(details)))(loop, details)
            loop.set_exception_handler(handshake_error)
            try:
                async with ClientSession() as client:
                    with self.assertRaises(ClientConnectorCertificateError):
                        await client.get(server.make_url("/v1/health"))
                    trusted = ssl.create_default_context(cafile=str(cert_path))
                    async with client.get(server.make_url("/v1/health"), ssl=trusted) as response:
                        self.assertEqual(200, response.status)
                        self.assertTrue((await response.json())["ready"])
            finally:
                await server.close()
                loop.set_exception_handler(previous_handler)
