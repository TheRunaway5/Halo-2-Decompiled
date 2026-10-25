"""TU1.5 defaults checked against unchanged original execution and HTTP commit."""
import asyncio
from copy import deepcopy
from dataclasses import replace
import json
from pathlib import Path
import unittest

from multiplayer.master_server.playlist_skill import (LATER_POLICY, LATER_DIGEST,
    MAX_XP, RatingPlayer, pair_delta, player_delta, raw_rank, rank_policy)
from multiplayer.master_server.clan_skill import RatingClan, clan_delta
from multiplayer.master_server.protocol import Problem
from .test_matchmaking_http import catalog
from . import test_match_transport as transport
from . import test_ranked_results as ranked


class LaterOriginalTests(unittest.TestCase):
    def test_original_controls(self):
        original = json.loads((Path(__file__).parent / "fixtures/title-update-1.5-rank-original.json").read_text())
        tables, digest = rank_policy(LATER_POLICY)
        self.assertEqual(tables, original["tables"])
        self.assertEqual(digest, LATER_DIGEST)
        controls = {r["name"]: r["actual"] for r in original["controls"]}
        actual = {}
        for name in controls:
            if name.startswith("pair own="):
                _, own, other, standing = name.split()
                a,b = map(int,standing.split("=")[1].split("/"))
                actual[name] = pair_delta(int(own.split("=")[1]),int(other.split("=")[1]),a,b,tables)
            if name.startswith("rank xp="):
                _, xp, previous = name.split()
                actual[name] = raw_rank(int(xp.split("=")[1]),int(previous.split("=")[1]),tables)
        for name,a,b in (("absent standings -1/-1",-1,-1),("absent standings 0/-1",0,-1),("absent standings -1/0",-1,0)):
            actual[name] = pair_delta(15,15,a,b,tables)
        def ratings(skills, places=None, teams=None, xp=None, **kwargs):
            players=[RatingPlayer(s,places[i] if places else i,teams[i] if teams else i,xp[i] if xp else 5000) for i,s in enumerate(skills)]
            return [player_delta(players,i,tables=tables,**kwargs) for i in range(len(players))]
        actual["FFA normal"] = ratings([20,17,18,15])
        actual["FFA low"] = ratings([10,7,8,5])
        actual["tie denominator"] = ratings([15,15,15],[0,0,1])
        actual["same team"] = ratings([15,15,15],[9,9,0],[0,0,1],team_places=[0,1])
        for name,flag,key in (("guest opponent exclusion",1,3),("playlist key exclusion",0,7)):
            players=[RatingPlayer(15,0,0),RatingPlayer(15,1,1,flags=flag,key=key)]
            actual[name] = [player_delta(players,i,tables=tables) for i in range(2)]
        actual["XP floor"] = ratings([15,15],[1,0],xp=[3,5000])
        actual["XP ceiling"] = ratings([15,15],xp=[MAX_XP-3,5000])
        for skills,places in (((15,15),(0,1)),((16,15),(0,1)),((15,15),(0,0))):
            clans=[RatingClan(s,p,5000) for s,p in zip(skills,places)]
            actual[f"clan aggregation {skills}/{places}"]=[clan_delta(clans,i,tables) for i in range(2)]
        self.assertEqual(set(actual),set(controls))
        for name,value in actual.items():
            with self.subTest(name=name): self.assertEqual(value,controls[name])


class LaterHttpTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = transport.MatchTransportTests.asyncSetUp
    session = transport.MatchTransportTests.session
    enqueue = transport.MatchTransportTests.enqueue
    pair = transport.MatchTransportTests.pair
    report = transport.MatchTransportTests.report
    running = ranked.RankedResultsTests.running
    review = ranked.RankedResultsTests.review

    @staticmethod
    def catalog_fixture():
        value=deepcopy(catalog())
        for row in value["playlists"]: row["rank_configuration"]=LATER_POLICY
        return value

    async def test_later_policy_frozen_at_admission_and_committed_atomically(self):
        for owner in ("one","two"): await self.session(owner)
        db=self.api.directory.store.db
        for session in self.api.sessions.values(): self.api.skills.snapshot(session.player,"ranked",LATER_POLICY)
        with db:
            db.execute("UPDATE playlist_skill SET raw_rank=15,xp=1800")
            db.execute("UPDATE playlist_ranks SET rank=16")
        _,descriptors,sockets,queues,host=await self.running()
        raw=self.report(descriptors);raw[0x370+0x94+0x90]=1
        digest=await self.review(descriptors,sockets,queues,host,raw)
        for socket in sockets: await socket.send_json({"confirm_result":digest})
        for queue in queues:
            self.assertEqual(await asyncio.wait_for(queue.get(),2),{"completed":digest})
            await asyncio.wait_for(queue.get(),2)
        rows=db.execute("SELECT configuration,table_digest,delta,xp FROM match_result_skill ORDER BY delta DESC").fetchall()
        self.assertEqual([tuple(r) for r in rows],[(LATER_POLICY,LATER_DIGEST,100,1900),(LATER_POLICY,LATER_DIGEST,-62,1738)])
        self.assertEqual(db.execute("SELECT count(*) FROM carnage_reports").fetchone()[0],1)
        self.assertEqual(db.execute("SELECT count(*) FROM carnage_player_summaries").fetchone()[0],2)

    async def test_existing_launch_skill_cannot_be_silently_reinterpreted(self):
        await self.session()
        who=next(iter(self.api.sessions.values())).player
        from multiplayer.master_server.playlist_skill import POLICY
        self.api.skills.snapshot(who,"separate-old-playlist",POLICY)
        with self.assertRaises(Problem) as caught:
            self.api.skills.snapshot(who,"separate-old-playlist",LATER_POLICY)
        self.assertEqual(caught.exception.status,409)
        self.assertEqual(tuple(self.api.directory.store.db.execute("SELECT configuration,xp FROM playlist_skill").fetchone()),(POLICY,0))
