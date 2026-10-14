"""HTTP history, source-detail decoding, guest exclusion and legacy migration."""
import asyncio
import hashlib
import json
import struct
import unittest
import uuid

from . import test_couch_online as couch
from . import test_match_transport as transport
from multiplayer.master_server.carnage_history import ReportHistory
from multiplayer.master_server.player_identity import VerifiedOwner
from multiplayer.master_server.protocol import Problem


class CarnageHistoryTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = couch.CouchOnlineTests.asyncSetUp
    catalog_fixture = staticmethod(couch.catalog)
    session = couch.CouchOnlineTests.session
    enqueue = couch.CouchOnlineTests.enqueue
    pair = transport.MatchTransportTests.pair
    report = transport.MatchTransportTests.report

    async def completed(self):
        async def verify(session, token, application):
            return VerifiedOwner("discord", "private-subject-"+token, token)
        self.api.directory.verify_owner = verify
        _, descriptors, sockets, queues, host = await self.pair()
        room = self.api.transport.rooms[descriptors[0]["id"]]
        for socket in sockets:
            await socket.send_json({"loaded": "1234567890abcdef:0000035"})
        for queue in queues:
            self.assertEqual((await asyncio.wait_for(queue.get(), 2))["phase"], "running")
        raw = self.report(descriptors)
        raw[0x13] = raw[0x68] = 1
        for slot, (player, _, team) in enumerate(room.players):
            guest = player in room.guests
            at = 0x370+slot*0x94
            raw[at+0x88], raw[at+0x90] = team, slot
            raw[at+0x4c:at+0x5c] = bytes([241 if guest else slot+1])*16
            for index, value in ((7,7),(9,2),(8,3),(11,1),(45+3,4),(77+5*8+4,11)):
                struct.pack_into("<H",raw,0xe30+2*(slot*437+index),30000 if guest else value)
            for other,(opponent,_,_) in enumerate(room.players):
                struct.pack_into("<HH",raw,0x44d0+4*(slot*16+other),30000 if guest or opponent in room.guests else 5,0)
            raw[0xcb0+team*0x18] = 1
            raw[0xcb0+team*0x18+1] = team
            struct.pack_into("<H",raw,0x48d0+2*(team*45+7),17)
        h = next(i for i,d in enumerate(descriptors) if d["slot"]==host)
        await sockets[h].send_json({"postgame":raw.hex()})
        for queue in queues:
            self.assertEqual(await asyncio.wait_for(queue.get(),2),{"postgame":raw.hex()})
        for socket in sockets:
            await socket.send_json({"confirm_result":hashlib.sha256(raw).hexdigest()})
        for queue in queues:
            self.assertIn("completed",await asyncio.wait_for(queue.get(),2))
            await asyncio.wait_for(queue.get(),2)
        return room, raw

    async def test_website_lookup_complete_details_and_guest_exclusion(self):
        room, raw = await self.completed()
        response = await self.client.get("/v1/history/players?discord_username=ONE")
        self.assertEqual(response.status,200)
        player = (await response.json())["players"][0]
        self.assertEqual(player["discord_username"],"one")
        self.assertNotIn(player["id"], {p for p,_,_ in room.players})
        history = await (await self.client.get("/v1/history/players/"+player["id"]+"/matches")).json()
        self.assertEqual(len(history["matches"]),1)
        self.assertEqual(history["matches"][0]["kills"],7)
        report = await (await self.client.get("/v1/history/matches/"+history["matches"][0]["id"])).json()
        self.assertEqual((report["admitted_players"],report["published_players"]),(8,2))
        self.assertEqual(len(report["players"]),2)
        self.assertEqual(report["variant_bytes"],raw[0x20:0x150].hex())
        for row in report["players"]:
            self.assertTrue(row["details_available"])
            self.assertEqual(row["medals"],[[3,4]])
            self.assertEqual(row["weapons"],[[5,4,11]])
            self.assertEqual(len(row["opponents"]),2)
            self.assertTrue(all(v["values"]==[5,0] for v in row["opponents"]))
        self.assertEqual(report["teams"][0]["general"],[[7,17]])
        encoded = json.dumps(report)
        for private in ["private-subject", "discord_token", "guest-", "Guest 2", "30000", *[p for p,_,_ in room.players]]:
            self.assertNotIn(private,encoded)
        db = self.api.directory.store.db
        self.assertEqual(db.execute("SELECT count(*) FROM carnage_players").fetchone()[0],2)
        self.assertEqual(db.execute("SELECT count(*) FROM carnage_opponents").fetchone()[0],4)
        self.assertEqual(db.execute("SELECT count(*) FROM carnage_general_stats WHERE value=30000").fetchone()[0],0)
        # Replayed completion cannot append another normalized report.
        self.api.results.commit(room)
        self.assertEqual(db.execute("SELECT count(*) FROM carnage_reports").fetchone()[0],1)

    async def test_current_verified_username_changes_without_rewriting_old_report(self):
        room,_ = await self.completed()
        db = self.api.directory.store.db
        who = db.execute("SELECT player FROM player_profiles WHERE discord_username='one'").fetchone()[0]
        with db:
            db.execute("UPDATE player_profiles SET discord_username='renamed_user',screen_name='New Pilot' WHERE player=?",(who,))
        self.assertEqual((await (await self.client.get("/v1/history/players?discord_username=one")).json())["players"],[])
        player = (await (await self.client.get("/v1/history/players?discord_username=renamed_user")).json())["players"][0]
        public = db.execute("SELECT public_id FROM carnage_reports WHERE match=?",(room.id,)).fetchone()[0]
        report = await (await self.client.get("/v1/history/matches/"+public)).json()
        row = next(r for r in report["players"] if r["id"]==player["id"])
        self.assertEqual((row["discord_username"],row["discord_username_at_match"]),("renamed_user","one"))
        self.assertEqual((row["screen_name"],row["screen_name_at_match"]),("New Pilot","Same Name"))

    async def test_detail_failure_rolls_back_summary_and_all_history_rows(self):
        _, descriptors, _, _, _ = await self.pair()
        room = self.api.transport.rooms[descriptors[0]["id"]]
        room.result = bytes(self.report(descriptors))
        room.result_digest = hashlib.sha256(room.result).hexdigest()
        room.confirmations = dict.fromkeys(room.tokens, room.result_digest)
        capture = self.api.results.history.capture
        def fail_after_details(room):
            capture(room)
            raise Problem(500, "Injected history storage failure")
        self.api.results.history.capture = fail_after_details
        with self.assertRaises(Problem):
            self.api.results.commit(room)
        db = self.api.directory.store.db
        for table in ("match_results", "match_result_players", "carnage_reports", "history_players",
                      "carnage_players", "carnage_general_stats", "carnage_medals", "carnage_weapon_stats"):
            self.assertEqual(db.execute("SELECT count(*) FROM "+table).fetchone()[0], 0, table)

    async def test_legacy_summaries_remain_honest_and_paginate_without_duplicates(self):
        room,_ = await self.completed()
        db = self.api.directory.store.db
        who = next(p for p,_,_ in room.players if p not in room.guests)
        with db:
            for i in range(5):
                key = str(uuid.uuid4())
                db.execute("INSERT INTO match_results VALUES(?,?,?,?,?,?,?,?,?)",(key,"old","v0","lockout","Slayer",0,100,"a"*64,"native-unanimous-v1"))
                db.execute("INSERT INTO match_result_players VALUES(?,?,?,?,?,?,?,?)",(key,who,0,3,3,1,0,0))
        ReportHistory(self.api.directory.store)
        public = db.execute("SELECT public_id FROM history_players WHERE player=?",(who,)).fetchone()[0]
        ids, before = [], None
        while True:
            url="/v1/history/players/"+public+"/matches?limit=2"+("&before="+before if before else "")
            response = await self.client.get(url);self.assertEqual(response.status,200)
            value = await response.json();ids.extend(r["id"] for r in value["matches"])
            before=value["next"]
            if not before:break
        self.assertEqual(len(ids),6);self.assertEqual(len(set(ids)),6)
        legacy = await (await self.client.get("/v1/history/matches/"+ids[-1])).json()
        self.assertEqual(legacy["format"],"legacy-summary-v1")
        self.assertIsNone(legacy["admitted_players"])
        self.assertIsNone(legacy["players"][0]["screen_name_at_match"])
        self.assertFalse(legacy["players"][0]["details_available"])
        self.assertEqual(legacy["players"][0]["weapons"],[])
        ReportHistory(self.api.directory.store)
        self.assertEqual(db.execute("SELECT count(*) FROM carnage_reports").fetchone()[0],6)

    async def test_bounded_read_only_routes_and_no_unknown_player_enrollment(self):
        for path in ("/v1/history/players?discord_username=x", "/v1/history/players?discord_username=one&extra=1",
                     "/v1/history/players?discord_username=one&discord_username=two"):
            self.assertEqual((await self.client.get(path)).status,400)
        self.assertEqual((await (await self.client.get("/v1/history/players?discord_username=unknown")).json())["players"],[])
        self.assertEqual(self.api.directory.store.db.execute("SELECT count(*) FROM players").fetchone()[0],0)
        self.assertEqual((await self.client.get("/v1/history/matches/"+str(uuid.uuid4()))).status,404)
        self.assertEqual((await self.client.post("/v1/history/players",json={})).status,405)
        await self.completed()
        public = self.api.directory.store.db.execute("SELECT public_id FROM history_players LIMIT 1").fetchone()[0]
        for query in ("limit=0","limit=51","limit=-1","before=bad!","before=e30","before=a","before=abcde","before=____"):
            self.assertEqual((await self.client.get("/v1/history/players/"+public+"/matches?"+query)).status,400)
