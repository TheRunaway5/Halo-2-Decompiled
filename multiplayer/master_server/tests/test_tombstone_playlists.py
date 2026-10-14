"""Revised Tombstone admission and real gathering/relay descriptors."""
import asyncio
import contextlib
import json
from pathlib import Path
import unittest
import uuid
from unittest.mock import patch
from aiohttp import WSMsgType

from . import test_matchmaking_http as baseline
from multiplayer.master_server.matchmaking_http import load_catalog


CATALOG = Path(__file__).resolve().parents[1] / "examples/matchmaking-tombstone.json"


class TombstonePlaylistTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = baseline.MatchmakingHttpTests.asyncSetUp
    catalog_fixture = staticmethod(lambda: json.loads(CATALOG.read_text()))

    async def session(self, owner="one", name="Chosen", *, maps=None, couch=True):
        body = dict(screen_name=name, maps=maps or ["lockout", "foundation", "highplains2"],
                    compatibility="halo2-native-queue-v10", language=0)
        if couch:
            body["local_players"] = [dict(screen_name=f"Guest {i}") for i in range(2, 5)]
        response = await self.client.post("/v1/matchmaking/session",
            headers={"Authorization": "Bearer " + owner}, json=body)
        self.assertEqual(response.status, 201, await response.text())
        return {"Authorization": "Bearer " + (await response.json())["token"]}

    async def enqueue(self, headers):
        policy = next(e.policy for e in self.api.entries if e.policy.id == self.selected)
        return await self.client.post("/v1/matchmaking/ticket", headers=headers,
            json=dict(playlist_id=policy.id, revision=policy.revision, request_id=str(uuid.uuid4())))

    def test_catalog_keeps_validation_queues_and_supported_engines(self):
        before = {e.policy.id: e for e in load_catalog(CATALOG.with_name("matchmaking-action-sack.json"))}
        after = {e.policy.id: e for e in load_catalog(CATALOG)}
        changed = {"casual-team", "ranked-team", "action-sack", "casual-slayer"}
        for key in before.keys() - changed:
            self.assertEqual(before[key], after[key])
        for key in changed:
            games = [g for g in after[key].policy.games if g.map == "highplains2"]
            self.assertEqual(len(games), 1)
            self.assertIn(games[0].variant, {"Slayer", "Team Slayer", "Fiesta Slayer"})
            self.assertGreater(games[0].weight, 0)
        self.assertEqual((after["casual-slayer"].policy.minimum_players,
                          after["casual-slayer"].policy.maximum_players), (6, 8))
        self.assertFalse(after["casual-slayer"].policy.ranked)

    def test_live_catalog_preserves_queues_and_activates_only_working_fiesta(self):
        before = load_catalog(CATALOG.with_name("matchmaking-action-sack.json"))
        live = load_catalog(CATALOG.with_name("matchmaking-tombstone-fiesta.json"))
        self.assertEqual(live[:-1], before[:-1])
        self.assertEqual(live[-1].policy.games[:-1], before[-1].policy.games)
        added = live[-1].policy.games[-1]
        self.assertEqual((added.map, added.variant, added.weight), ("highplains2", "Fiesta Slayer", 1))
        self.assertEqual(live[-1].required_compatibility, "halo2-native-queue-v10")

    async def test_disc_only_and_old_revision_do_not_admit_unloadable_rotations(self):
        for maps in (["lockout", "foundation"], ["lockout", "foundation", "highplains"]):
            headers = await self.session(maps=maps, couch=False)
            response = await self.client.get("/v1/matchmaking/playlists", headers=headers)
            rows = {p["id"]: p for p in (await response.json())["playlists"]}
            self.assertTrue(rows["casual"]["eligible"])
            for self.selected in ("casual-team", "ranked-team", "action-sack", "casual-slayer"):
                self.assertFalse(rows[self.selected]["eligible"])
                self.assertEqual((await self.enqueue(headers)).status, 400)
        self.assertFalse(self.api.core.tickets)

    async def check_couch_selection(self, playlist, variant, team_mode):
        self.selected = playlist
        headers = [await self.session(owner) for owner in ("one", "two")]
        async def echo(socket):
            async for message in socket:
                self.assertEqual(message.type, WSMsgType.TEXT)
                value = json.loads(message.data)
                if "probe" in value:
                    await socket.send_json({"echo": value["probe"]})
                elif "ping" in value:
                    await socket.send_json({"pong": value["ping"]})
        async def stop(task):
            task.cancel()
            with contextlib.suppress(asyncio.CancelledError):
                await task
        for header in headers:
            socket = await self.client.ws_connect("/v1/matchmaking/qos", headers=header)
            self.addAsyncCleanup(socket.close)
            self.addAsyncCleanup(stop, asyncio.create_task(echo(socket)))
        # Force the selected map only; use the actual catalog, gathering owner,
        # authenticated physical connections, admission and relay descriptor.
        def tombstone(population, **kwargs):
            return [next(g for g in population if g.map == "highplains2")]
        with patch.object(self.api.core.rng, "choices", side_effect=tombstone):
            for header in headers:
                self.assertEqual((await self.enqueue(header)).status, 201)
            async with asyncio.timeout(4):
                while not self.api.core.proposals:
                    for header in headers:
                        response = await self.client.get("/v1/matchmaking/ticket", headers=header)
                        self.assertEqual(response.status, 200, await response.text())
                    await asyncio.sleep(.005)
            descriptors = []
            for header in headers:
                response = await self.client.get("/v1/matchmaking/ticket", headers=header)
                match = (await response.json())["match"]
                self.assertEqual((match["map"], match["variant"]), ("highplains2", variant))
                response = await self.client.get("/v1/matchmaking/transport", headers=header)
                self.assertEqual(response.status, 200, await response.text())
                descriptors.append(await response.json())
        self.assertEqual(descriptors[0]["id"], descriptors[1]["id"])
        self.assertNotEqual(descriptors[0]["token"], descriptors[1]["token"])
        for row in descriptors:
            self.assertEqual(row["team_mode"], team_mode)
            self.assertEqual(len(row["local_slots"]), 4)
            self.assertEqual(len(row["players"]), 8)

    async def test_team_slayer_two_couches_select_revised_tombstone(self):
        await self.check_couch_selection("casual-team", "Team Slayer", True)

    async def test_slayer_two_couches_select_revised_tombstone(self):
        await self.check_couch_selection("casual-slayer", "Slayer", False)

    async def test_fiesta_two_couches_select_revised_tombstone(self):
        await self.check_couch_selection("action-sack", "Fiesta Slayer", False)
