import json
import tempfile
import unittest
import uuid
from pathlib import Path
from aiohttp.test_utils import TestClient, TestServer
from multiplayer.master_server.protocol import Problem
from multiplayer.master_server.service import Settings, create_app
from multiplayer.master_server.matchmaking_http import load_catalog


def catalog():
    base = dict(revision="lab-1", description="Local validation playlist; rotation and timing are custom.",
                team_size=0, minimum_players=2, maximum_players=2, maximum_party=1,
                games=[dict(id="slayer", map="lockout", variant="Slayer", weight=1)],
                stages=[dict(after_seconds=0, rank_numerator=1, rank_denominator=1, minimum_connection=50)],
                provenance="Lab fixture. Not an original Halo 2 playlist rotation.")
    return dict(version=1, playlists=[
        base | dict(id="ranked", name="Ranked lab", ranked=True, quickmatch=True,
                    skill=dict(kind="bungie-2007", high_rank_radius=14)),
        base | dict(id="casual", name="Casual lab", ranked=False, quickmatch=False, skill=dict(kind="custom", radius=6))])


class MatchmakingHttpTests(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name) / "playlists.json"
        self.path.write_text(json.dumps(getattr(self, "catalog_fixture", catalog)()))
        self.verified = []
        async def verifier(session, token, application):
            self.assertEqual(application, "1558522073272426519")
            if token not in ("one", "two", "three"):
                raise Problem(401, "Invalid test Discord authorization")
            self.verified.append(token)
            return "discord", "private-subject-" + token
        self.app = create_app(Settings(state=Path(self.temp.name), public_url="http://127.0.0.1:8080",
                                      allow_http=True, matchmaking_catalog=str(self.path)), verifier)
        self.client = TestClient(TestServer(self.app))
        await self.client.start_server()
        self.addAsyncCleanup(self.client.close)
        self.api = next(iter(self.app.values())).matchmaking

    async def session(self, owner="one", name="Chosen Pilot", maps=None):
        response = await self.client.post("/v1/matchmaking/session", headers={"Authorization": "Bearer " + owner},
            json=dict(screen_name=name, maps=maps or ["lockout"], compatibility="test-v1"))
        self.assertEqual(response.status, 201, await response.text())
        return {"Authorization": "Bearer " + (await response.json())["token"]}

    async def enqueue(self, headers, playlist="ranked", **extra):
        body = dict(playlist_id=playlist, revision="lab-1", request_id=str(uuid.uuid4())) | extra
        return await self.client.post("/v1/matchmaking/ticket", headers=headers, json=body)

    async def test_native_screen_name_capacity(self):
        for name in ("a" * 32, "😀" * 16, "\ud800"):
            response = await self.client.post("/v1/matchmaking/session", headers={"Authorization": "Bearer one"},
                json=dict(screen_name=name, maps=["lockout"], compatibility="test-v1"))
            self.assertEqual(response.status, 400)
        for compatibility in ("halo2-native-queue-v3", "halo2-native-queue-v6"):
            response = await self.client.post("/v1/matchmaking/session", headers={"Authorization": "Bearer one"},
                json=dict(screen_name="Pilot", maps=["lockout"], compatibility=compatibility))
            self.assertEqual(response.status, 409)

    async def test_identity_privacy_names_private_skill_and_idempotence(self):
        a = await self.session()
        b = await self.session("two", "Another Pilot")
        response = await self.client.get("/v1/matchmaking/playlists", headers=a)
        rows = (await response.json())["playlists"]
        self.assertEqual(rows[0]["level"], 1)
        self.assertIsNone(rows[1]["level"])
        for injection in (dict(player_id="forged"), dict(rank=50), dict(players=[]), dict(connection=100)):
            self.assertEqual((await self.enqueue(a, **injection)).status, 400)
        key = str(uuid.uuid4())
        self.assertEqual((await self.enqueue(a, request_id=key)).status, 201)
        self.assertEqual((await self.enqueue(a, request_id=key)).status, 201)
        self.assertEqual(len(self.api.core.tickets), 1)
        self.assertEqual((await self.enqueue(b)).status, 201)
        response = await self.client.get("/v1/matchmaking/ticket", headers=a)
        body = await response.json()
        self.assertEqual(body["state"], "checking_connections")
        self.assertEqual({p["name"] for p in body["match"]["players"]}, {"Chosen Pilot", "Another Pilot"})
        for private in ("private-subject", "player_id", "proposal_id", "ticket_id", '"rank"', "token"):
            self.assertNotIn(private, json.dumps(body))
        self.assertEqual(self.verified, ["one", "two"])
        proposal = next(iter(self.api.core.proposals.values()))
        with self.assertRaises(Problem):
            self.api.core.acknowledge(proposal.id, next(iter(self.api.sessions.values())).player)
        ids = [s.player for s in self.api.sessions.values()]
        self.api.core.qos_lease = 1
        self.api.core.connection(ids[0], ids[1], 100)
        response = await self.client.get("/v1/matchmaking/ticket", headers=a)
        self.assertEqual((await response.json())["state"], "awaiting_transport")
        now = self.api.core.now()
        self.api.core.clock = lambda: now + 2
        response = await self.client.get("/v1/matchmaking/ticket", headers=a)
        self.assertEqual((await response.json())["state"], "checking_connections")
        with self.assertRaises(Problem): self.api.core.acknowledge(proposal.id, ids[0])
        self.assertEqual((await self.client.delete("/v1/matchmaking/ticket", headers=a)).status, 200)
        self.assertEqual(len(self.api.core.tickets), 1)
        self.assertEqual(len(self.api.core.proposals), 0)
        self.assertEqual((await self.enqueue(a, request_id=key)).status, 409)

    async def test_session_rotation_revoke_expiry_and_no_cross_player_cancel(self):
        a, b = await self.session(), await self.session("two")
        await self.enqueue(a)
        await self.enqueue(b, "casual")
        replacement = await self.session(name="Renamed Pilot")
        self.assertEqual((await self.client.get("/v1/matchmaking/ticket", headers=a)).status, 401)
        self.assertEqual(len(self.api.core.tickets), 1)
        await self.enqueue(replacement)
        await self.client.delete("/v1/matchmaking/session", headers=replacement)
        self.assertEqual((await self.enqueue(replacement)).status, 401)
        self.assertEqual(len(self.api.core.tickets), 1)
        for session in self.api.sessions.values():
            session.expires = 0
        self.api.cleanup()
        self.assertEqual(len(self.api.core.tickets), 0)
        self.assertEqual(len(self.api.requests), 0)

    async def test_eligibility_revision_isolation_and_authoritative_skill(self):
        a = await self.session(maps=["ascension"])
        rows = await (await self.client.get("/v1/matchmaking/playlists", headers=a)).json()
        self.assertFalse(rows["playlists"][0]["eligible"])
        self.assertEqual((await self.enqueue(a)).status, 400)
        a = await self.session()
        self.assertEqual((await self.enqueue(a, revision="stale")).status, 409)
        player = next(s.player for s in self.api.sessions.values() if s.screen_name == "Chosen Pilot")
        self.api.rank(player, "ranked")
        with self.api.directory.store.db:
            self.api.directory.store.db.execute("UPDATE playlist_ranks SET rank=40 WHERE player=?", (player,))
        await self.enqueue(a)
        self.assertEqual(next(iter(self.api.core.tickets.values())).players[0].rank, 40)
        b = await self.session("two")
        await self.enqueue(b, "casual")
        self.assertEqual(len(self.api.core.proposals), 0)
        self.assertEqual((await self.enqueue(a, "casual")).status, 409)
        self.assertEqual((await self.client.get("/v1/matchmaking/playlists")).status, 401)
        self.assertEqual((await self.client.post("/v1/matchmaking/session", headers={"Authorization": "Bearer forged"},
             json=dict(screen_name="Pilot", maps=["lockout"], compatibility="test-v1"))).status, 401)

    def test_catalog_fail_closed(self):
        value = catalog()
        value["playlists"][0]["skill"].pop("high_rank_radius")
        self.path.write_text(json.dumps(value))
        # Low ranks have published bounds; unknown high-rank limits stay unset.
        self.assertIsNone(load_catalog(self.path)[0].policy.high_rank_radius)
        for mutate in (lambda v: v.update(version=2), lambda v: v["playlists"][1].update(quickmatch=True),
                       lambda v: v["playlists"][0]["skill"].update(radius=5)):
            value = catalog(); mutate(value); self.path.write_text(json.dumps(value))
            with self.assertRaises(Problem): load_catalog(self.path)


class DisabledMatchmakingTests(unittest.IsolatedAsyncioTestCase):
    async def test_absent_catalog_disables_queue_without_breaking_directory(self):
        with tempfile.TemporaryDirectory() as directory:
            app = create_app(Settings(state=Path(directory), public_url="http://127.0.0.1:8080", allow_http=True))
            async with TestClient(TestServer(app)) as client:
                self.assertEqual((await client.get("/v1/health")).status, 200)
                self.assertEqual((await client.get("/v1/servers")).status, 200)
                self.assertEqual((await client.get("/v1/matchmaking/playlists")).status, 503)


class PrivateRawSkillHttpTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = MatchmakingHttpTests.asyncSetUp
    session = MatchmakingHttpTests.session
    enqueue = MatchmakingHttpTests.enqueue

    @staticmethod
    def catalog_fixture():
        value = catalog()
        for entry in value["playlists"]:
            entry["rank_configuration"] = "retail-launch-2004"
        return value

    async def test_ranked_and_hidden_casual_use_private_raw_skill_past_display_cap(self):
        from multiplayer.master_server.playlist_skill import TABLES
        a, b = await self.session(), await self.session("two", "Another Pilot")
        players = [s.player for s in self.api.sessions.values()]
        for player, raw in zip(players, (80, 100)):
            for playlist in ("ranked", "casual"):
                self.api.rank(player, playlist)
                with self.api.directory.store.db:
                    self.api.directory.store.db.execute(
                        "UPDATE playlist_skill SET raw_rank=?,xp=? WHERE player=? AND playlist=?",
                        (raw, TABLES["xp_thresholds"][raw], player, playlist))
        for headers in (a, b):
            response = await self.client.get("/v1/matchmaking/playlists", headers=headers)
            rows = (await response.json())["playlists"]
            self.assertEqual(rows[0]["level"], 50)
            self.assertIsNone(rows[1]["level"])
        for playlist in ("ranked", "casual"):
            self.assertEqual((await self.enqueue(a, playlist)).status, 201)
            self.assertEqual((await self.enqueue(b, playlist)).status, 201)
            self.assertEqual([t.players[0].raw_rank for t in self.api.core.tickets.values()], [80, 100])
            self.assertEqual([t.players[0].rank for t in self.api.core.tickets.values()], [50, 50])
            self.assertFalse(self.api.core.proposals)
            for headers in (a, b):
                response = await self.client.get("/v1/matchmaking/ticket", headers=headers)
                body = await response.json()
                self.assertEqual(body["state"], "searching")
                for private in ("raw_rank", "private-subject", '"rank"', "player_id", "token"):
                    self.assertNotIn(private, json.dumps(body))
            for headers in (a, b):
                self.assertEqual((await self.client.delete("/v1/matchmaking/ticket", headers=headers)).status, 200)
