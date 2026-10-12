import asyncio
import json
from pathlib import Path
import tempfile
import unittest
import uuid

from aiohttp.test_utils import TestClient, TestServer
from multiplayer.master_server.service import Settings, create_app
from multiplayer.master_server.player_identity import VerifiedOwner, player
from multiplayer.master_server.protocol import Problem
from .test_matchmaking_http import catalog


class ClansHttpTests(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name)
        (self.path / "catalog.json").write_text(json.dumps(catalog()))
        async def verifier(session, token, application):
            if token not in ("one", "two", "three", "four"):
                raise Problem(401, "Invalid Discord authorization")
            return VerifiedOwner("discord", "private-discord-subject-" + token, "pilot_" + token)
        self.verifier = verifier
        self.settings = Settings(state=self.path, public_url="http://127.0.0.1:8080", allow_http=True,
                                 matchmaking_catalog=str(self.path / "catalog.json"), rate_per_minute=100000)
        await self.start()
        self.a, self.b, self.c, self.d = [await self.login(who) for who in ("one", "two", "three", "four")]

    async def start(self):
        self.app = create_app(self.settings, self.verifier)
        self.client = TestClient(TestServer(self.app))
        await self.client.start_server()
        self.addAsyncCleanup(self.client.close)
        self.api = next(iter(self.app.values())).clans

    async def login(self, who="one", name="Chosen Pilot"):
        response = await self.client.post("/v1/account/session", headers={"Authorization": "Bearer " + who}, json={"screen_name": name})
        self.assertEqual(response.status, 201, await response.text())
        return {"Authorization": "Bearer " + (await response.json())["token"]}

    async def command(self, headers, action, status=200, **kw):
        body = dict(action=action, request_id=str(uuid.uuid4())) | kw
        response = await self.client.post("/v1/clans/commands", headers=headers, json=body)
        value = await response.json()
        self.assertEqual(response.status, status, value)
        return value

    async def view(self, headers):
        response = await self.client.get("/v1/clans", headers=headers)
        self.assertEqual(response.status, 200, await response.text())
        return await response.json()

    async def join(self, target, username):
        await self.command(self.a, "invite", username=username)
        invite = (await self.view(target))["invitations"][0]["id"]
        return await self.command(target, "accept", target=invite)

    async def make_clan(self):
        return await self.command(self.a, "create", name="Test Clan")

    async def test_creation_identity_names_and_privacy(self):
        for name in ("", "name!", "1234567890123456", "   "):
            await self.command(self.a, "create", 400, name=name)
        first = await self.make_clan()
        self.assertEqual(first["clan"]["role"], 3)
        self.assertEqual(first["clan"]["members"][0]["name"], "Chosen Pilot")
        for private in ("private-discord", "pilot_one", "token", "subject", "provider"):
            self.assertNotIn(private, json.dumps(first))
        await self.command(self.b, "create", 409, name="TEST CLAN")
        await self.command(self.a, "create", 409, name="Other Clan")
        renamed = await self.login("one", "My Own Name")
        self.assertEqual((await self.view(renamed))["clan"]["members"][0]["name"], "My Own Name")
        self.assertEqual((await self.client.get("/v1/clans", headers=self.a)).status, 401)
        self.a = renamed
        await self.command(self.a, "rename", 400, name="Other Clan")
        await self.command(self.a, "invite", 400, username="Chosen Pilot")
        forged = await self.client.post("/v1/account/session", headers={"Authorization": "Bearer one"}, json={"screen_name": "Fake", "discord_username": "pilot_two"})
        self.assertEqual(forged.status, 400)

    async def test_invitation_accept_decline_block_and_cancel(self):
        await self.make_clan()
        await self.command(self.a, "invite", username="pilot_two")
        iid = (await self.view(self.b))["invitations"][0]["id"]
        await self.command(self.c, "accept", 404, target=iid)
        await self.command(self.b, "decline", target=iid)
        self.assertEqual((await self.view(self.b))["invitations"], [])
        await self.command(self.a, "invite", username="pilot_two")
        iid = (await self.view(self.b))["invitations"][0]["id"]
        await self.command(self.a, "cancel", target=iid)
        await self.command(self.b, "accept", 404, target=iid)
        await self.command(self.a, "invite", username="pilot_two")
        iid = (await self.view(self.b))["invitations"][0]["id"]
        await self.command(self.b, "block", target=iid)
        await self.command(self.a, "invite", 403, username="pilot_two")
        joined = await self.join(self.c, "pilot_three")
        self.assertEqual(joined["clan"]["role"], 1)
        await self.command(self.c, "create", 409, name="New Clan")
        await self.command(self.a, "invite", 409, username="pilot_three")

    async def test_original_role_matrix_and_multiple_overlords(self):
        await self.make_clan()
        await self.join(self.b, "pilot_two")
        await self.join(self.c, "pilot_three")
        roster = (await self.view(self.a))["clan"]["members"]
        bid = next(m["id"] for m in (await self.view(self.b))["clan"]["members"] if m["self"])
        cid = next(m["id"] for m in (await self.view(self.c))["clan"]["members"] if m["self"])
        aid = next(m["id"] for m in roster if m["self"])
        await self.command(self.b, "role", 403, target=cid, role=0, confirmed=True)
        await self.command(self.a, "role", target=bid, role=2, confirmed=True)
        await self.command(self.b, "role", target=cid, role=2, confirmed=True)
        await self.command(self.b, "remove", 403, target=cid)
        await self.command(self.b, "role", 403, target=aid, role=0, confirmed=True)
        await self.command(self.b, "role", 403, target=cid, role=3, confirmed=True)
        await self.command(self.a, "role", 409, target=bid, role=3, confirmed=False)
        await self.command(self.a, "role", target=bid, role=3, confirmed=True)
        await self.command(self.a, "remove", 403, target=bid)
        await self.command(self.a, "role", 403, target=bid, role=2, confirmed=True)
        self.assertEqual(sum(m["role"] == 3 for m in (await self.view(self.b))["clan"]["members"]), 2)
        await self.command(self.a, "leave")
        await self.command(self.b, "leave", 409)
        await self.command(self.b, "remove", target=cid)
        await self.command(self.b, "leave")
        self.assertIsNone((await self.view(self.b))["clan"])
        self.assertEqual(self.api.db.execute("SELECT count(*) FROM clans").fetchone()[0], 0)

    async def test_peon_permissions_messages_and_recruitment(self):
        await self.make_clan()
        await self.join(self.b, "pilot_two")
        bid = next(m["id"] for m in (await self.view(self.b))["clan"]["members"] if m["self"])
        await self.command(self.b, "broadcast", message="A clan message")
        self.assertEqual((await self.view(self.a))["clan"]["messages"][0]["body"], "A clan message")
        await self.command(self.a, "role", target=bid, role=0, confirmed=True)
        await self.command(self.b, "invite", 403, username="pilot_three")
        await self.command(self.b, "broadcast", 403, message="Not allowed")
        self.assertEqual(len((await self.view(self.b))["clan"]["messages"]), 1)
        self.assertIsNone((await self.view(self.c))["clan"])

    async def test_replay_expiry_durability_and_queue_independence(self):
        rid = str(uuid.uuid4())
        first = await self.command(self.a, "create", name="Persistent", request_id=rid)
        again = await self.command(self.a, "create", name="Persistent", request_id=rid)
        self.assertEqual(first, again)
        await self.command(self.a, "create", 409, name="Different", request_id=rid)
        response = await self.client.post("/v1/matchmaking/session", headers={"Authorization":"Bearer one"}, json=dict(screen_name="Queue Pilot", maps=["lockout"], compatibility="test"))
        queue = {"Authorization":"Bearer " + (await response.json())["token"]}
        self.a = await self.login("one")
        self.assertEqual((await self.client.get("/v1/matchmaking/playlists", headers=queue)).status,200)
        await self.client.delete("/v1/matchmaking/session", headers=queue)
        self.assertIsNotNone((await self.view(self.a))["clan"])
        await self.client.close()
        await self.start()
        self.a = await self.login("one")
        self.assertEqual((await self.view(self.a))["clan"]["name"],"Persistent")
        self.api.clock = lambda: 10**20
        self.assertEqual((await self.client.get("/v1/clans",headers=self.a)).status,401)

    async def test_single_clan_race_capacity_and_invite_expiry(self):
        await self.make_clan()
        await self.command(self.c,"create",name="Other Clan")
        await self.command(self.a,"invite",username="pilot_two")
        await self.command(self.c,"invite",username="pilot_two")
        invitations = (await self.view(self.b))["invitations"]
        responses = await asyncio.gather(*[self.client.post("/v1/clans/commands",headers=self.b,json=dict(action="accept",target=i["id"],request_id=str(uuid.uuid4()))) for i in invitations])
        self.assertEqual(sorted(r.status for r in responses),[200,404])
        await self.command(self.a,"invite",username="pilot_four")
        self.api.wall=lambda: 10**12
        self.assertEqual((await self.view(self.d))["invitations"],[])
        cid = (await self.view(self.a))["clan"]["id"]
        with self.api.db:
            count = self.api.db.execute("SELECT count(*) FROM clan_members WHERE clan=?",(cid,)).fetchone()[0]
            for i in range(100-count):
                pid=player(self.api.db,("fixture",str(i)),"Extra")
                self.api.db.execute("INSERT INTO clan_members VALUES(?,?,1,0)",(pid,cid))
        await self.command(self.a,"invite",409,username="pilot_four")

    async def test_revoked_session_cannot_mutate_and_no_public_rosters(self):
        await self.make_clan()
        self.assertEqual((await self.client.get("/v1/clans")).status,401)
        await self.client.delete("/v1/account/session",headers=self.a)
        await self.command(self.a,"leave",401)
        self.assertEqual(self.api.db.execute("SELECT count(*) FROM clans").fetchone()[0],1)

    async def test_presence_is_recent_authenticated_activity_and_opt_in_schema(self):
        await self.make_clan()
        await self.join(self.b, "pilot_two")
        legacy = (await self.view(self.a))["clan"]["members"]
        self.assertTrue(all(set(m) == {"id", "name", "role", "self"} for m in legacy))
        queue_api = self.api.directory.matchmaking
        now = max(s.seen for s in self.api.sessions.values()) + 1
        self.api.clock = queue_api.clock = lambda: now

        async def presence():
            response = await self.client.get("/v1/clans?presence=1", headers=self.a)
            self.assertEqual(response.status, 200, await response.text())
            value = await response.json()
            self.assertNotIn("private-discord", json.dumps(value))
            return {m["self"]: m["presence"] for m in value["clan"]["members"]}

        self.assertEqual(await presence(), {True: "online", False: "online"})
        now += 31
        # The viewer's authenticated poll renews only its own observation. A
        # fifteen-minute bearer lease must not keep a stopped peer online.
        self.assertEqual(await presence(), {True: "online", False: "offline"})
        await self.view(self.b)
        self.assertEqual(await presence(), {True: "online", False: "online"})
        await self.client.delete("/v1/account/session", headers=self.b)
        self.assertEqual(await presence(), {True: "online", False: "offline"})

        response = await self.client.post("/v1/matchmaking/session",
            headers={"Authorization": "Bearer two"},
            json=dict(screen_name="Chosen Pilot", maps=["lockout"], compatibility="test"))
        self.assertEqual(response.status, 201, await response.text())
        queue = {"Authorization": "Bearer " + (await response.json())["token"]}
        self.assertEqual(await presence(), {True: "online", False: "online"})
        now += 31
        self.assertEqual(await presence(), {True: "online", False: "offline"})
        self.assertEqual((await self.client.get("/v1/matchmaking/playlists", headers=queue)).status, 200)
        self.assertEqual(await presence(), {True: "online", False: "online"})
        await self.client.delete("/v1/matchmaking/session", headers=queue)
        self.assertEqual(await presence(), {True: "online", False: "offline"})
        self.assertEqual((await self.client.get("/v1/clans?presence=1")).status, 401)
        outsider = await self.client.get("/v1/clans?presence=1", headers=self.c)
        self.assertIsNone((await outsider.json())["clan"])
        await self.command(self.a, "broadcast", 400, message="No forged status", presence="playing")
