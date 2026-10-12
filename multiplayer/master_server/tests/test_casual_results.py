"""Hidden retail skill goes through real admission and unanimous result commit."""
import asyncio
from copy import deepcopy
from dataclasses import replace
import json
import sqlite3
import unittest

from multiplayer.master_server.playlist_skill import POLICY, TABLE_DIGEST
from multiplayer.master_server.matchmaking_http import load_catalog
from .test_matchmaking_http import catalog
from . import test_match_transport as transport
from . import test_ranked_results as ranked


class CasualResultsTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = transport.MatchTransportTests.asyncSetUp
    session = transport.MatchTransportTests.session
    pair = transport.MatchTransportTests.pair
    report = transport.MatchTransportTests.report
    running = ranked.RankedResultsTests.running
    review = ranked.RankedResultsTests.review

    @staticmethod
    def catalog_fixture():
        value = catalog()
        value["playlists"] = [deepcopy(row) for row in value["playlists"]]
        for row in value["playlists"]:
            row["rank_configuration"] = POLICY
        return value

    async def enqueue(self, headers, playlist="casual", **extra):
        return await transport.MatchTransportTests.enqueue(self, headers, playlist, **extra)

    async def test_hidden_skill_commits_once_drives_next_search_and_survives_visibility_change(self):
        headers, descriptors, sockets, queues, host = await self.running()
        raw = self.report(descriptors)
        raw[0x370+0x94+0x90] = 1
        digest = await self.review(descriptors, sockets, queues, host, raw)
        db = self.api.directory.store.db
        await sockets[0].send_json({"confirm_result": digest})
        await asyncio.sleep(.05)
        self.assertEqual(db.execute("SELECT count(*) FROM match_result_skill").fetchone()[0], 0)
        await sockets[1].send_json({"confirm_result": digest})
        for queue in queues:
            self.assertEqual(await asyncio.wait_for(queue.get(), 2), {"completed": digest})
            await asyncio.wait_for(queue.get(), 2)
        self.assertEqual(tuple(db.execute("SELECT playlist,ranked FROM match_results").fetchone()), ("casual", 0))
        self.assertEqual([tuple(r) for r in db.execute(
            "SELECT configuration,table_digest,delta,raw_rank,xp FROM match_result_skill ORDER BY delta DESC")],
            [(POLICY, TABLE_DIGEST, 100, 1, 100), (POLICY, TABLE_DIGEST, 0, 0, 0)])
        room = self.api.transport.rooms[descriptors[0]["id"]]
        self.api.results.commit(room)
        await sockets[0].send_json({"confirm_result": digest})
        await asyncio.sleep(.05)
        self.assertEqual(db.execute("SELECT sum(xp) FROM playlist_skill").fetchone()[0], 100)
        self.assertEqual(db.execute("SELECT count(*) FROM match_result_skill").fetchone()[0], 2)
        for header, descriptor in zip(headers, descriptors):
            body = await (await self.client.get("/v1/matchmaking/playlists", headers=header)).json()
            self.assertEqual(body["playlists"][0]["level"], 1)  # Independent ranked playlist.
            self.assertIsNone(body["playlists"][1]["level"])
            for private in ("player", "subject", "xp", "raw_rank", "table_digest"):
                self.assertNotIn('"'+private+'"', json.dumps(body))
            response = await self.enqueue(header)
            self.assertEqual(response.status, 201, await response.text())
            self.assertNotIn('"rank"', await response.text())
            player = room.players[descriptor["slot"]][0]
            ticket = self.api.ticket(player)
            self.assertEqual(ticket.players[0].rank, 2 if descriptor["slot"] == 0 else 1)
        with sqlite3.connect(self.api.directory.settings.state / "registry.sqlite3") as reopened:
            self.assertEqual(reopened.execute("SELECT sum(xp) FROM playlist_skill").fetchone()[0], 100)
            self.assertEqual(reopened.execute("SELECT count(*) FROM match_result_skill").fetchone()[0], 2)
        # Bungie's 2005 Team Snipers promotion exposed its existing hidden rank.
        # Visibility and catalog revision changes must not reset that same hopper.
        self.api.entries = tuple(replace(e, policy=replace(e.policy, ranked=True, revision="promotion-2"))
                                 if e.policy.id == "casual" else e for e in self.api.entries)
        for header, descriptor in zip(headers, descriptors):
            body = await (await self.client.get("/v1/matchmaking/playlists", headers=header)).json()
            self.assertEqual(body["playlists"][1]["level"], 2 if descriptor["slot"] == 0 else 1)
        self.assertEqual(db.execute("SELECT sum(xp) FROM playlist_skill").fetchone()[0], 100)

    async def test_unknown_hidden_skill_configuration_is_rejected(self):
        value = self.catalog_fixture()
        value["playlists"][1]["rank_configuration"] = "unknown-casual-policy"
        self.path.write_text(json.dumps(value))
        from multiplayer.master_server.protocol import Problem
        with self.assertRaises(Problem):
            load_catalog(self.path)

    async def test_catalog_restricts_unbounded_retry_to_final_casual_stage(self):
        from multiplayer.master_server.protocol import Problem
        value = self.catalog_fixture()
        stage = dict(after_seconds=30, rank_numerator=1, rank_denominator=1,
                     minimum_connection=50, unbounded_skill=True)
        value["playlists"][1]["stages"].append(stage)
        self.path.write_text(json.dumps(value))
        self.assertTrue(load_catalog(self.path)[1].policy.stages[-1].unbounded_skill)
        for target, flag, seconds in ((0, True, 30), (1, "yes", 30), (1, True, 0)):
            with self.subTest(target=target, flag=flag, seconds=seconds):
                value = self.catalog_fixture()
                value["playlists"][target]["stages"].append(stage | dict(
                    unbounded_skill=flag, after_seconds=seconds))
                self.path.write_text(json.dumps(value))
                with self.assertRaises(Problem):
                    load_catalog(self.path)
