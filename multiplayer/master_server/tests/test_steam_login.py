"""Real HTTP handoff with an independent fixture OpenID signature verifier."""
import base64
import hashlib
import hmac
import json
from pathlib import Path
import tempfile
import time
import unittest
from datetime import datetime, timezone
from unittest.mock import patch

from aiohttp.test_utils import TestClient, TestServer
from multiplayer.master_server.service import Settings, create_app
from multiplayer.master_server.steam_login import OP, NS
from multiplayer.master_server.player_identity import VerifiedOwner
from .test_matchmaking_http import catalog


class Body:
    def __init__(self, data): self.data = data
    async def read(self, maximum): return self.data[:maximum]
    async def iter_chunked(self, maximum):
        for i in range(0, len(self.data), maximum): yield self.data[i:i + maximum]


class Response:
    def __init__(self, data, status=200):
        self.status, self.headers, self.content_length = status, {}, len(data)
        self.content = Body(data)
    async def __aenter__(self): return self
    async def __aexit__(self, *args): pass


class SteamLoginTests(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self):
        self.temp = tempfile.TemporaryDirectory(); self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        (self.root / "catalog.json").write_text(json.dumps(catalog()))
        self.settings = Settings(state=self.root, public_url="https://master.halo2forever.net",
                                 behind_tls_proxy=True, steam_enabled=True, rate_per_minute=100000,
                                 matchmaking_catalog=str(self.root / "catalog.json"))
        async def discord(session, token, application):
            return VerifiedOwner("discord", "retained-discord-owner", "retained_pilot")
        self.app = create_app(self.settings, discord)
        self.client = TestClient(TestServer(self.app)); await self.client.start_server()
        self.addAsyncCleanup(self.client.close)
        self.directory = next(iter(self.app.values())); self.steam = self.directory.steam
        self.now = time.time(); self.steam.clock = lambda: self.now
        self.verifier = "v" * 43
        self.key = b"independent-fixture-openid-signature"
        self.calls = []
        self.signature_valid = True

        def provider(url, *, data, allow_redirects):
            self.assertEqual((url, allow_redirects), (OP, False))
            self.assertEqual(data["openid.mode"], "check_authentication")
            self.calls.append(dict(data))
            valid = self.signature_valid and hmac.compare_digest(data["openid.sig"], self.signature(data))
            return Response(("ns:" + NS + "\nis_valid:" + ("true" if valid else "false") + "\n").encode())
        p = patch.object(self.directory.session, "post", provider); p.start(); self.addCleanup(p.stop)

    def signature(self, values):
        signed = "".join(k + ":" + values["openid." + k] + "\n" for k in values["openid.signed"].split(","))
        return base64.b64encode(hmac.new(self.key, signed.encode(), hashlib.sha256).digest()).decode()

    async def start(self):
        challenge = base64.urlsafe_b64encode(hashlib.sha256(self.verifier.encode()).digest()).rstrip(b"=").decode()
        r = await self.client.post("/v1/auth/steam/start", json={"challenge": challenge})
        self.assertEqual(r.status, 201, await r.text()); value = await r.json()
        self.assertEqual(r.headers["Cache-Control"], "no-store")
        self.assertTrue(value["url"].startswith(OP + "?"))
        return value["device"]

    def assertion(self, device, subject="76561197960287930"):
        nonce = datetime.fromtimestamp(self.now, timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ") + device
        value = {"openid.ns": NS, "openid.mode": "id_res", "openid.op_endpoint": OP,
                 "openid.claimed_id": "https://steamcommunity.com/openid/id/" + subject,
                 "openid.identity": "https://steamcommunity.com/openid/id/" + subject,
                 "openid.return_to": self.steam.callback_url(device), "openid.response_nonce": nonce,
                 "openid.assoc_handle": "fixture-association",
                 "openid.signed": "op_endpoint,claimed_id,identity,return_to,response_nonce,assoc_handle"}
        value["openid.sig"] = self.signature(value)
        return value

    async def complete(self, subject="76561197960287930"):
        device = await self.start()
        r = await self.client.get("/v1/auth/steam/callback/" + device, params=self.assertion(device, subject))
        self.assertEqual(r.status, 200, await r.text()); self.assertNotIn(subject, await r.text())
        self.assertEqual(r.url.path,"/v1/auth/steam/complete")
        self.assertFalse(r.url.query)
        r = await self.client.post("/v1/auth/steam/poll", json={"device": device, "verifier": self.verifier})
        self.assertEqual(r.status, 200, await r.text())
        return device, await r.json()

    async def test_verified_identity_private_handoff_and_clans(self):
        device, result = await self.complete()
        self.assertEqual(result["state"], "signed_in")
        self.assertEqual(result["identity"]["id"], "76561197960287930")
        token = result["token"]; self.assertTrue(token.startswith("h2s_"))
        headers = {"Authorization": "Bearer " + token}
        r = await self.client.post("/v1/account/session", headers=headers, json={"screen_name": "Chosen Halo Name"})
        self.assertEqual(r.status, 201, await r.text())
        session = (await r.json())["token"]
        r = await self.client.get("/v1/clans", headers={"Authorization": "Bearer " + session})
        self.assertEqual(r.status, 200, await r.text())
        self.assertNotIn("76561197960287930", await r.text())
        row = self.steam.db.execute("SELECT screen_name,discord_username,steam_name FROM player_profiles").fetchone()
        self.assertEqual(tuple(row), ("Chosen Halo Name", "", "Steam Player"))
        self.assertNotIn(token, repr(self.steam.db.execute("SELECT * FROM steam_login_sessions").fetchall()))
        r = await self.client.post("/v1/auth/steam/poll", json={"device": device, "verifier": self.verifier})
        self.assertEqual(r.status, 401)

    async def test_wrong_proof_does_not_consume_real_signin(self):
        device = await self.start()
        r = await self.client.post("/v1/auth/steam/poll", json={"device": device, "verifier": "x" * 43})
        self.assertEqual(r.status, 401)
        r = await self.client.post("/v1/auth/steam/poll", json={"device": device, "verifier": self.verifier})
        self.assertEqual(await r.json(), {"state": "pending"})
        self.assertFalse(self.calls)

    async def test_dual_proof_retains_discord_clan_and_rejects_duplicate_couch_owner(self):
        r = await self.client.post("/v1/account/session", headers={"Authorization":"Bearer legacy"}, json={"screen_name":"Retained Pilot"})
        legacy = (await r.json())["token"]
        import uuid
        r = await self.client.post("/v1/clans/commands", headers={"Authorization":"Bearer " + legacy},
                                   json={"request_id":str(uuid.uuid4()),"action":"create","name":"Retained Clan"})
        self.assertEqual(r.status,200,await r.text())
        old = self.steam.db.execute("SELECT id FROM players WHERE provider='discord'").fetchone()[0]
        _, result = await self.complete()
        headers={"Authorization":"Bearer " + result["token"]}
        r = await self.client.post("/v1/account/session",headers=headers,json={"screen_name":"New Halo Alias"})
        self.assertEqual(r.status,201,await r.text())
        r = await self.client.post("/v1/account/link-discord",headers=headers,json={"discord_token":"legacy"})
        self.assertEqual(r.status,200,await r.text());self.assertEqual(await r.json(),{"state":"linked"})
        linked = self.steam.db.execute("SELECT player FROM player_identities WHERE provider='steam'").fetchone()[0]
        self.assertEqual(linked,old)
        r = await self.client.post("/v1/account/session",headers=headers,json={"screen_name":"Chosen Screen Name"})
        session=(await r.json())["token"]
        r = await self.client.get("/v1/clans",headers={"Authorization":"Bearer " + session})
        self.assertEqual((await r.json())["clan"]["name"],"Retained Clan")
        self.assertNotIn("76561197960287930",await r.text())
        r = await self.client.post("/v1/matchmaking/session",headers=headers,json={"screen_name":"Chosen Screen Name","maps":["lockout"],
            "compatibility":"halo2-native-queue-v12","local_players":[{"screen_name":"Duplicate","discord_token":"legacy"}]})
        self.assertEqual(r.status,400,await r.text())
        _, another=await self.complete("76561197960287931")
        r=await self.client.post("/v1/account/link-discord",headers={"Authorization":"Bearer "+another["token"]},json={"discord_token":"legacy"})
        self.assertEqual(r.status,409,await r.text())

    async def test_steam_voice_preferences_and_existing_data_are_preserved(self):
        _, result=await self.complete()
        headers={"Authorization":"Bearer " + result["token"]}
        r=await self.client.post("/v1/account/session",headers=headers,json={"screen_name":"Steam Pilot"})
        self.assertEqual(r.status,201,await r.text())
        steam_id=self.steam.db.execute("SELECT id FROM players WHERE provider='steam'").fetchone()[0]
        r=await self.client.post("/v1/account/session",headers={"Authorization":"Bearer legacy"},json={"screen_name":"Discord Pilot"})
        discord_id=self.steam.db.execute("SELECT id FROM players WHERE provider='discord'").fetchone()[0]
        self.assertTrue(self.directory.voice_blocks.allowed(steam_id,discord_id))
        self.assertFalse(self.directory.voice_blocks.allowed(steam_id,"unknown-player"))
        r=await self.client.put("/v1/account/voice-blocks",headers=headers,json={"blocked":[]})
        self.assertEqual(r.status,200,await r.text())
        # A persisted rank must not be discarded to combine independently played accounts.
        with self.steam.db:
            self.steam.db.execute("INSERT INTO playlist_skill VALUES(?,?,?,?,?)",(steam_id,"ranked","test",0,0))
        r=await self.client.post("/v1/account/link-discord",headers=headers,json={"discord_token":"legacy"})
        self.assertEqual(r.status,409,await r.text())
        self.assertIsNotNone(self.steam.db.execute("SELECT 1 FROM playlist_skill WHERE player=?",(steam_id,)).fetchone())

    async def test_forged_signed_identity_and_duplicate_fields_rejected(self):
        device = await self.start(); values = self.assertion(device)
        values["openid.claimed_id"] = values["openid.identity"] = "https://steamcommunity.com/openid/id/76561197960287931"
        r = await self.client.get("/v1/auth/steam/callback/" + device, params=values)
        self.assertEqual(r.status, 401)
        self.assertEqual(self.steam.db.execute("SELECT count(*) FROM steam_login_sessions").fetchone()[0], 0)
        device = await self.start(); values = list(self.assertion(device).items())
        values.append(("openid.identity", "https://steamcommunity.com/openid/id/76561197960287931"))
        r = await self.client.get("/v1/auth/steam/callback/" + device, params=values)
        self.assertEqual(r.status, 400)

    async def test_wrong_destination_nonce_and_unsigned_fields_rejected_before_provider(self):
        for key, value in [("openid.return_to", "https://attacker.example/"),
                           ("openid.op_endpoint", "https://attacker.example/"),
                           ("openid.response_nonce", "2000-01-01T00:00:00Zold"),
                           ("openid.signed", "op_endpoint,identity")]:
            device = await self.start(); values = self.assertion(device); values[key] = value
            r = await self.client.get("/v1/auth/steam/callback/" + device, params=values)
            self.assertEqual(r.status, 400, await r.text())
        self.assertFalse(self.calls)

    async def test_expiry_retained_discord_rows_and_verified_couch_seat(self):
        r = await self.client.post("/v1/account/session", headers={"Authorization": "Bearer legacy"}, json={"screen_name": "Old Pilot"})
        self.assertEqual(r.status, 201)
        before = tuple(self.steam.db.execute("SELECT id,provider,subject FROM players").fetchone())
        _, first = await self.complete(); _, second = await self.complete("76561197960287931")
        r = await self.client.post("/v1/matchmaking/session", headers={"Authorization": "Bearer " + first["token"]},
                                  json={"screen_name": "Primary", "maps": ["lockout"], "compatibility": "halo2-native-queue-v9",
                                        "language": 0, "local_players": [{"screen_name": "Signed Seat", "identity_token": second["token"]},
                                                                        {"screen_name": "Guest 3"}, {"screen_name": "Guest 4"}]})
        self.assertEqual(r.status, 201, await r.text())
        seated = next(iter(self.directory.matchmaking.sessions.values())).local_players
        self.assertEqual([p.guest for p in seated], [False, False, True, True])
        self.assertEqual(self.steam.db.execute("SELECT count(*) FROM players").fetchone()[0], 3)
        self.assertEqual(tuple(self.steam.db.execute("SELECT id,provider,subject FROM players WHERE id=?", (before[0],)).fetchone()), before)
        self.now += 28801
        r = await self.client.post("/v1/account/session", headers={"Authorization": "Bearer " + first["token"]}, json={"screen_name": "Expired"})
        self.assertEqual(r.status, 401)
