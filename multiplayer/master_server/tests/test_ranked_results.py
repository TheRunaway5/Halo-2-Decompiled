import asyncio
from dataclasses import replace
import hashlib
import json
import sqlite3
import unittest

from multiplayer.master_server.playlist_skill import POLICY, TABLE_DIGEST
from multiplayer.master_server.protocol import Problem
from . import test_match_transport as transport


class RankedResultsTests(unittest.IsolatedAsyncioTestCase):
    session = transport.MatchTransportTests.session
    enqueue = transport.MatchTransportTests.enqueue
    pair = transport.MatchTransportTests.pair
    report = transport.MatchTransportTests.report

    async def asyncSetUp(self):
        await transport.MatchTransportTests.asyncSetUp(self)
        self.api.entries = tuple(replace(e, rank_configuration=POLICY) if e.policy.ranked else e
                                 for e in self.api.entries)

    async def running(self):
        headers, descriptors, sockets, queues, host = await self.pair()
        for socket in sockets:
            await socket.send_json({"loaded": "1234567890abcdef:000001c"})
        for queue in queues:
            self.assertEqual((await asyncio.wait_for(queue.get(),2))["phase"],"running")
        return headers, descriptors, sockets, queues, host

    async def review(self, descriptors, sockets, queues, host, raw):
        h = next(i for i,d in enumerate(descriptors) if d["slot"] == host)
        await sockets[h].send_json({"postgame":raw.hex()})
        for queue in queues:
            self.assertEqual(await asyncio.wait_for(queue.get(),2),{"postgame":raw.hex()})
        return hashlib.sha256(raw).hexdigest()

    async def test_unanimous_progression_private_refresh_idempotence_and_restart(self):
        headers, descriptors, sockets, queues, host = await self.running()
        raw = self.report(descriptors)
        raw[0x370+0x94+0x90] = 1  # Slot0 wins, slot1 loses.
        digest = await self.review(descriptors,sockets,queues,host,raw)
        db = self.api.directory.store.db
        await sockets[0].send_json({"confirm_result":digest})
        await asyncio.sleep(.05)
        self.assertEqual(db.execute("SELECT count(*) FROM match_result_skill").fetchone()[0],0)
        self.assertEqual({r[0] for r in db.execute("SELECT xp FROM playlist_skill")},{0})
        await sockets[1].send_json({"confirm_result":digest})
        for queue in queues:
            self.assertEqual(await asyncio.wait_for(queue.get(),2),{"completed":digest})
            await asyncio.wait_for(queue.get(),2)  # Authenticated voice membership.
        room = self.api.transport.rooms[descriptors[0]["id"]]
        rows = db.execute("SELECT configuration,table_digest,effective_rank,previous_rank,previous_xp,delta,raw_rank,xp "
                          "FROM match_result_skill ORDER BY delta DESC").fetchall()
        self.assertEqual([tuple(r) for r in rows],[(POLICY,TABLE_DIGEST,0,0,0,100,1,100),(POLICY,TABLE_DIGEST,0,0,0,0,0,0)])
        self.api.results.commit(room)
        await sockets[0].send_json({"confirm_result":digest})
        await asyncio.sleep(.05)
        self.assertEqual(db.execute("SELECT sum(xp) FROM playlist_skill").fetchone()[0],100)
        for header, descriptor in zip(headers,descriptors):
            body = await (await self.client.get("/v1/matchmaking/playlists",headers=header)).json()
            self.assertEqual(body["playlists"][0]["level"],2 if descriptor["slot"] == 0 else 1)
            self.assertIsNone(body["playlists"][1]["level"])
            for private in ("player", "subject", "xp", "raw_rank", "table_digest"):
                self.assertNotIn('"'+private+'"',json.dumps(body))
        with sqlite3.connect(self.api.directory.settings.state / "registry.sqlite3") as reopened:
            self.assertEqual(reopened.execute("SELECT sum(xp) FROM playlist_skill").fetchone()[0],100)
            self.assertEqual(reopened.execute("SELECT count(*) FROM match_result_skill").fetchone()[0],2)

    async def test_team_skill_uses_team_standing_and_frozen_authoritative_ranks(self):
        for owner in ("one","two"):
            await self.session(owner)
        db = self.api.directory.store.db
        for session in self.api.sessions.values():
            self.api.skills.snapshot(session.player,"ranked",POLICY)
        with db:
            db.execute("UPDATE playlist_skill SET raw_rank=15,xp=5500")
            db.execute("UPDATE playlist_ranks SET rank=16")
        self.api.core.playlists["ranked"] = replace(self.api.core.playlists["ranked"],team_size=1)
        _,descriptors,sockets,queues,host = await self.running()
        raw = self.report(descriptors)
        raw[0x13] = raw[0x68] = 1
        for slot,row in enumerate(descriptors[0]["players"]):
            at = 0x370+slot*0x94
            raw[at+0x88] = row["team"]
            raw[at+0x90] = 1-slot  # Reverse individual order to distinguish owners.
            at = 0xcb0+row["team"]*0x18
            raw[at],raw[at+1] = 1,slot
        digest = await self.review(descriptors,sockets,queues,host,raw)
        for socket in sockets: await socket.send_json({"confirm_result":digest})
        for queue in queues:
            self.assertEqual(await asyncio.wait_for(queue.get(),2),{"completed":digest})
            await asyncio.wait_for(queue.get(),2)
        self.assertEqual({r[0] for r in db.execute("SELECT delta FROM match_result_skill")},{100,-100})
        self.assertEqual({r[0] for r in db.execute("SELECT xp FROM playlist_skill")},{5400,5600})
        self.assertEqual({r[0] for r in db.execute("SELECT rank FROM playlist_ranks")},{16})  # Original hysteresis.

    async def test_changed_skill_rolls_back_entire_result_transaction(self):
        _,descriptors,sockets,queues,host = await self.running()
        room = self.api.transport.rooms[descriptors[0]["id"]]
        raw = self.report(descriptors)
        raw[0x370+0x94+0x90] = 1
        digest = await self.review(descriptors,sockets,queues,host,raw)
        db = self.api.directory.store.db
        with db:
            db.execute("UPDATE playlist_skill SET xp=1 WHERE player=?",(room.players[0][0],))
        for socket in sockets: await socket.send_json({"confirm_result":digest})
        for queue in queues: self.assertIsNone(await asyncio.wait_for(queue.get(),2))
        self.assertEqual(db.execute("SELECT count(*) FROM match_results").fetchone()[0],0)
        self.assertEqual(db.execute("SELECT count(*) FROM match_result_players").fetchone()[0],0)
        self.assertEqual(db.execute("SELECT count(*) FROM match_result_skill").fetchone()[0],0)
        self.assertEqual(db.execute("SELECT sum(xp) FROM playlist_skill").fetchone()[0],1)

    async def test_legacy_level_requires_known_xp_before_migration(self):
        await self.session()
        player = next(iter(self.api.sessions.values())).player
        db = self.api.directory.store.db
        with db:
            db.execute("INSERT INTO playlist_ranks VALUES(?,?,?)",(player,"ranked",40))
        with self.assertRaises(Problem) as error:
            self.api.rank(player,"ranked")
        self.assertEqual(error.exception.status,409)
        self.assertEqual(db.execute("SELECT count(*) FROM playlist_skill").fetchone()[0],0)
        self.assertEqual(db.execute("SELECT rank FROM playlist_ranks").fetchone()[0],40)
        with db: db.execute("UPDATE playlist_ranks SET rank=1")
        self.assertEqual(self.api.rank(player,"ranked"),1)
        self.assertEqual(tuple(db.execute("SELECT raw_rank,xp FROM playlist_skill").fetchone()),(0,0))
