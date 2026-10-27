from dataclasses import replace
import hashlib
import json
from pathlib import Path
import struct
import unittest

from multiplayer.master_server.match_results import ORIGINAL_PRESETS
from multiplayer.master_server.protocol import Problem
from . import test_match_transport as transport


class OriginalPresetRegistryTests(unittest.TestCase):
    def test_registry_matches_original_executed_preset_buffers(self):
        path=Path(__file__).resolve().parents[3]/"tests/fixtures/multiplayer_engines_original_tu15.json"
        if not path.is_file():
            self.skipTest("Original reference preset buffers are not installed")
        raw=path.read_bytes()
        self.assertEqual(hashlib.sha256(raw).hexdigest(),"c84eed61c3fcdac7788329917f764cb0f43523c9d800c68ee471fde21b171d19")
        fixture=json.loads(raw)
        self.assertEqual(fixture["original_code_sha256"],"21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")
        observed={}
        for row in fixture["variants"]:
            source=bytes.fromhex(row["raw"]);self.assertEqual(len(source),304)
            engine=struct.unpack_from("<I",source,0x44)[0];self.assertEqual(engine,row["source_type"])
            observed[row["name"]]=(engine,bool(source[0x48]&1))
        self.assertEqual(ORIGINAL_PRESETS,observed)


class OriginalResultFamilyTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp=transport.MatchTransportTests.asyncSetUp
    session=transport.MatchTransportTests.session
    enqueue=transport.MatchTransportTests.enqueue
    pair=transport.MatchTransportTests.pair
    report=transport.MatchTransportTests.report

    async def test_seven_source_families_preserve_roster_engine_and_team_guards(self):
        _,descriptors,_,_,_=await self.pair()
        room=self.api.transport.rooms[descriptors[0]["id"]]
        for name in ("Slayer","Oddball","2 on 1","King","CTF Classic","Multi Bomb","3 Plots","Team Ball","Team King"):
            with self.subTest(name=name):
                engine,teams=ORIGINAL_PRESETS[name]
                room.proposal=replace(room.proposal,game=replace(room.proposal.game,variant=name));room.team_mode=teams
                raw=self.report(descriptors);struct.pack_into("<I",raw,0x64,engine)
                raw[0x13]=raw[0x68]=int(teams)
                for slot,(_,_,team) in enumerate(room.players):
                    raw[0x370+slot*0x94+0x88]=team if teams else 255
                    if teams:raw[0xcb0+team*0x18]=1
                self.assertEqual(self.api.results.validate(room,raw.hex())[0],raw)
                wrong=raw.copy();struct.pack_into("<I",wrong,0x64,3 if engine==2 else 2)
                with self.assertRaises(Problem):self.api.results.validate(room,wrong.hex())
                for at in (0,0x11,0x13,0x68,0x370+2,0x370+0x88):
                    wrong=raw.copy();wrong[at]^=1
                    with self.assertRaises(Problem):self.api.results.validate(room,wrong.hex())
                if engine!=2:
                    room.team_mode=not teams;wrong=raw.copy();wrong[0x13]=wrong[0x68]=int(not teams)
                    with self.assertRaises(Problem):self.api.results.validate(room,wrong.hex())
                    room.team_mode=teams

    async def test_unknown_custom_names_remain_slayer_only(self):
        _,descriptors,_,_,_=await self.pair();room=self.api.transport.rooms[descriptors[0]["id"]]
        room.proposal=replace(room.proposal,game=replace(room.proposal.game,variant="Custom named Slayer"))
        raw=self.report(descriptors);self.assertEqual(self.api.results.validate(room,raw.hex())[0],raw)
        for engine in (1,3,4,7,8,9):
            struct.pack_into("<I",raw,0x64,engine)
            with self.assertRaises(Problem):self.api.results.validate(room,raw.hex())
