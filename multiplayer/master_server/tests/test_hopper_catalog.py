"""Public hopper configuration, original draw ABI and actual finalizer routes.

Authenticated identity/QoS and played-world acceptance remain separate. These
tests do not award statistics or contact EOS/Steam/Discord.
"""
import ctypes
import asyncio
import contextlib
from dataclasses import replace
import hashlib
import json
from pathlib import Path
import struct
from types import SimpleNamespace
import unittest
import uuid
from aiohttp import WSMsgType

from multiplayer.master_server.couch_players import NATIVE_ACCURACY as NATIVE_HOPPERS
from multiplayer.master_server.gathering_sessions import Gathering, GatheringSessions
from multiplayer.master_server.hopper_selection import HopperSelector, native
from multiplayer.master_server.matchmaking import Coordinator, Player
from multiplayer.master_server.matchmaking_http import load_catalog
from multiplayer.master_server.protocol import Problem
from multiplayer.master_server.search_lifecycle import GatheringClock
from . import test_matchmaking_http as baseline
from . import test_match_transport as transport

ROOT=Path(__file__).resolve().parents[3]
CATALOG=ROOT/'multiplayer/master_server/examples/matchmaking-online.json'


class HopperCatalogTests(unittest.TestCase):
    def test_all_public_hoppers_and_published_families_have_bound_settings(self):
        entries=load_catalog(CATALOG)
        self.assertEqual([e.name for e in entries],['Rumble Pit','Double Team','Team Slayer','Team Skirmish',
            'Team Training','Team Snipers','Team Hardcore','Team SWAT','Big Team Battle','Minor Clan Match',
            'Major Clan Match','Multi-Team','Action Sack','Lab Test'])
        self.assertEqual([e.policy.id for e in entries if e.quickmatch],['rumble-pit'])
        families=set()
        for entry in entries:
            if entry.policy.id!='lab-test':
                self.assertNotIn('lab',entry.name.lower())
                self.assertNotIn('lab',entry.description.lower())
            self.assertEqual(entry.required_compatibility,NATIVE_HOPPERS)
            self.assertIsNotNone(entry.policy.hopper)
            for game in entry.policy.games:
                self.assertNotIn(game.map,('highplains2','triplicate2'))
                if game.settings:
                    raw=bytes.fromhex(game.settings);self.assertEqual(len(raw),304)
                    families.add(struct.unpack_from('<i',raw,0x44)[0])
                    self.assertEqual(bool(raw[0x48]&1),bool(entry.policy.team_size))
                else:self.assertIn(game.variant,('Fiesta Slayer','Infection Slayer'))
        self.assertEqual(families,{1,2,3,4,8,9})
        # The dated target pools do not publish Juggernaut. Do not invent a
        # playlist row just to make a seven-engine implementation count green.

    def test_every_row_is_reachable_through_original_service_abi_without_losing_options(self):
        manifest=json.loads((ROOT/'multiplayer/master_server/data/online-hopper-frequencies-v1.json').read_text())
        self.assertEqual(manifest['frequency_domain']['rng_high16_values'],65536)
        self.assertEqual(manifest['frequency_domain']['map_availability'],
                         'All catalog maps available to every group member')
        for entry,expected in zip(load_catalog(CATALOG),manifest['playlists'],strict=True):
            policy=entry.policy;player=Player('one','Chosen',1,frozenset(g.map for g in policy.games),0)
            total=expected['weight_total'];cumulative=0
            self.assertEqual(sum(expected['map_draws_of_65536'].values()),65536)
            self.assertEqual(sum(expected['variant_draws_of_65536'].values()),65536)
            for game,row in zip(policy.games,expected['rows'],strict=True):
                # Invert the original LCG to choose the first high16 value in
                # this row's interval. There is no mocked selector.
                high=(cumulative*65536+total-1)//total
                seed=((high<<16)-1013904223)*4276115653&0xffffffff
                selected=HopperSelector(seed).choose(policy.games,(player,))
                self.assertIs(selected,game)
                self.assertEqual(selected.id,row['id'])
                self.assertEqual(hashlib.sha256(bytes.fromhex(selected.settings)).hexdigest() if selected.settings else None,
                                 row['settings_sha256'])
                cumulative+=max(1,min(1000,game.weight))

    def test_hopper_rank_map_is_validated_before_any_player_queues(self):
        policy=load_catalog(CATALOG)[0].policy
        raw=bytearray.fromhex(policy.hopper)
        raw[0x51c:0x59c]=bytes(128)
        with self.assertRaises(Problem):
            replace(policy,hopper=raw.hex())

    def test_all_public_hoppers_reach_actual_gathering_team_and_game_finalizer(self):
        for entry in load_catalog(CATALOG):
            with self.subTest(playlist=entry.name):
                policy=entry.policy;core=Coordinator((policy,),clock=lambda:100.)
                maps=frozenset(game.map for game in policy.games)
                size=policy.team_size if policy.team_size else 1
                group=[]
                for party in range(policy.maximum_players//size):
                    players=tuple(Player(f'p{party}-{seat}','Chosen',1,maps,0) for seat in range(size))
                    extra={'clan':f'clan-{party}','clan_rank':0} if policy.clan_match else {}
                    group.append(core.enqueue(f'party-{party}',policy.id,NATIVE_HOPPERS,players,**extra))
                owner=GatheringSessions(SimpleNamespace(core=core,search_qos=None))
                gathering=Gathering('gathering',tuple(t.id for t in group),100,group[0].players[0].id,0,
                                    None,GatheringClock(100000,100000))
                gathering=owner._restore_fixture(gathering)
                proposal=owner.proposal(policy,gathering)
                self.assertIsNotNone(proposal)
                self.assertIn(proposal.game,policy.games)
                self.assertEqual(sum(map(len,proposal.teams)),policy.maximum_players)
                self.assertEqual(tuple(map(len,proposal.teams)),(size,)*(policy.team_count if policy.team_size else policy.maximum_players))
                self.assertEqual(proposal.revision,policy.revision)

    def test_tombstone_uses_mcc_key_in_suitable_pools_and_action_sack_mode_totals_match(self):
        entries={e.policy.id:e.policy for e in load_catalog(CATALOG)}
        for name in ('rumble-pit','team-slayer','team-skirmish','big-team-battle','team-snipers','action-sack'):
            self.assertTrue(any(g.map=='highplains' for g in entries[name].games),name)
        self.assertTrue(any(g.map=='highplains' and struct.unpack_from('<i',bytes.fromhex(g.settings),0x44)[0]==1
                            for g in entries['team-skirmish'].games))
        totals={name:sum(g.weight for g in entries['action-sack'].games if g.variant==name)
                for name in ('Fiesta Slayer','Infection Slayer')}
        self.assertEqual(totals,{'Fiesta Slayer':6,'Infection Slayer':6})

    def test_published_hopper_roles_and_large_sniper_maps_are_preserved(self):
        entries={e.policy.id:e.policy for e in load_catalog(CATALOG)}
        self.assertNotIn(2,{struct.unpack_from('<i',bytes.fromhex(g.settings),0x44)[0]
                             for g in entries['team-skirmish'].games})
        doubles={struct.unpack_from('<i',bytes.fromhex(g.settings),0x44)[0] for g in entries['double-team'].games}
        self.assertTrue({1,2,3,4}.issubset(doubles))
        self.assertTrue({'burial_mounds','headlong','waterworks'}.issubset(
                        {g.map for g in entries['team-snipers'].games}))
        self.assertTrue({'midship','beavercreek','lockout','warlock','elongation','backwash','colossus','deltatap','cyclotron','gemini'}.issubset(
                        {g.map for g in entries['team-hardcore'].games}))

    def test_lab_is_separate_unranked_and_varied(self):
        entries=load_catalog(CATALOG)
        entry=next(e for e in entries if e.policy.id=='lab-test')
        policy=entry.policy
        self.assertFalse(entry.quickmatch or policy.ranked)
        self.assertIsNone(entry.rank_configuration)
        self.assertEqual((policy.minimum_players,policy.maximum_players,policy.maximum_party),(2,16,16))
        self.assertEqual(len(policy.games),96)
        self.assertEqual(len({g.map for g in policy.games}),12)
        self.assertTrue({'lockout','midship','highplains','derelict','coagulation'}.issubset({g.map for g in policy.games}))
        variants={g.variant for g in policy.games}
        self.assertEqual(variants,{'Team Slayer','Team Snipers','Team SWAT','Multi Flag CTF',
                                   'Single Bomb','Team Ball','Team King','3 Plots'})
        self.assertEqual({struct.unpack_from('<i',bytes.fromhex(g.settings),0x44)[0] for g in policy.games},{1,2,3,4,8,9})

    def test_lab_minimum_and_short_countdown_use_actual_gathering_finalizer(self):
        policy=next(e.policy for e in load_catalog(CATALOG) if e.policy.id=='lab-test')
        for sizes in ((1,),(1,1),(2,),(2,2)):
            with self.subTest(party_sizes=sizes):
                now=[100.]
                core=Coordinator((policy,),clock=lambda:now[0]);group=[]
                for party,size in enumerate(sizes):
                    players=tuple(Player(f'p{party}-{seat}','Chosen',1,frozenset(('highplains',)),0,
                                         guest=seat>0) for seat in range(size))
                    group.append(core.enqueue(f'party-{party}',policy.id,NATIVE_HOPPERS,players))
                owner=GatheringSessions(SimpleNamespace(core=core,search_qos=None))
                gathering=Gathering('lab',tuple(t.id for t in group),100,group[0].players[0].id,0,
                                    None,GatheringClock(100000,100000))
                gathering=owner._restore_fixture(gathering)
                self.assertIsNone(owner.proposal(policy,gathering))
                now[0]=100.999
                self.assertIsNone(owner.proposal(policy,gathering))
                now[0]=101.
                result=owner.proposal(policy,gathering)
                if sum(sizes)==1:
                    self.assertIsNone(result)
                else:
                    self.assertIsNotNone(result)
                    self.assertEqual(tuple(map(len,result.teams)),(sum(sizes)//2,)*2)
                    self.assertEqual(result.game.map,'highplains')

    def test_group_availability_is_enforced_and_empty_pool_does_not_spend_rng(self):
        policy=load_catalog(CATALOG)[0].policy
        games=policy.games
        player=Player('one','Chosen',1,frozenset(('unknown_map',)),0)
        selector=HopperSelector(17)
        self.assertIsNone(selector.choose(games,(player,)))
        self.assertEqual(selector.seed,17)
        with self.assertRaises(Problem):Coordinator((policy,)).enqueue('party',policy.id,NATIVE_HOPPERS,(player,))
        # A party missing the other maps still queues and selects only its
        # actual common map. The old all-map prerequisite fails this control.
        player=replace(player,maps=frozenset(('highplains',)))
        core=Coordinator((policy,));core.enqueue('party',policy.id,NATIVE_HOPPERS,(player,))
        self.assertEqual(core.hoppers.choose(games,(player,)).map,'highplains')

    def test_invalid_abi_inputs_leave_every_output_unchanged(self):
        weights=(ctypes.c_int32*1)(1);available=(ctypes.c_uint8*1)(2)
        outputs=[ctypes.c_uint32(0x12345678) for _ in range(4)]
        for count in (1,101):
            self.assertEqual(native()(9,count,weights,available,*(ctypes.byref(v) for v in outputs)),2)
            self.assertEqual([v.value for v in outputs],[0x12345678]*4)


class HopperHttpTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp=baseline.MatchmakingHttpTests.asyncSetUp
    catalog_fixture=staticmethod(lambda:json.loads(CATALOG.read_text()))
    report=transport.MatchTransportTests.report

    async def session(self,owner,compatibility=NATIVE_HOPPERS,*,couch=True):
        body=dict(screen_name='Player '+owner,maps=['highplains'],language=0,compatibility=compatibility)
        if couch:body['local_players']=[dict(screen_name='Guest '+owner)]
        response=await self.client.post('/v1/matchmaking/session',headers={'Authorization':'Bearer '+owner},json=body)
        self.assertEqual(response.status,201,await response.text())
        return {'Authorization':'Bearer '+(await response.json())['token']}

    async def test_old_client_cannot_enter_complete_option_hoppers(self):
        auth=await self.session('one','halo2-native-queue-v14')
        response=await self.client.get('/v1/matchmaking/playlists',headers=auth)
        self.assertFalse(any(row['eligible'] for row in (await response.json())['playlists']))
        response=await self.client.post('/v1/matchmaking/ticket',headers=auth,
            json=dict(playlist_id='team-training',revision='h2f-final-era-v1',request_id=str(uuid.uuid4())))
        self.assertEqual(response.status,409)

    async def connect_qos(self,auth):
        async def echo(socket):
            async for message in socket:
                self.assertEqual(message.type,WSMsgType.TEXT)
                value=json.loads(message.data)
                if 'probe' in value:await socket.send_json({'echo':value['probe']})
                elif 'ping' in value:await socket.send_json({'pong':value['ping']})
        async def stop(task):
            task.cancel()
            with contextlib.suppress(asyncio.CancelledError):await task
        socket=await self.client.ws_connect('/v1/matchmaking/qos',headers=auth)
        self.addAsyncCleanup(socket.close);self.addAsyncCleanup(stop,asyncio.create_task(echo(socket)))

    async def test_real_couch_admission_preserves_full_row_through_eos_descriptor_and_results(self):
        headers=[await self.session(owner) for owner in ('one','two')]
        for auth in headers:
            await self.connect_qos(auth)
            response=await self.client.post('/v1/matchmaking/ticket',headers=auth,
                json=dict(playlist_id='team-training',revision='h2f-final-era-v1',request_id=str(uuid.uuid4())))
            self.assertEqual(response.status,201,await response.text())
        # Explicit clock fixture only: no45-second real wait, no altered
        # selector, assignment, settings, authentication or completed world.
        async with asyncio.timeout(5):
            while not self.api.core.proposals:
                for gathering in self.api.gatherings.sessions.values():
                    gathering.clock.growth=(self.api.gatherings.tick(self.api.core.now())-60000)&0xffffffff
                for auth in headers:
                    response=await self.client.get('/v1/matchmaking/ticket',headers=auth)
                    self.assertEqual(response.status,200,await response.text())
                await asyncio.sleep(.005)
        matches=[];descriptors=[]
        for auth in headers:
            response=await self.client.get('/v1/matchmaking/ticket',headers=auth);matches.append((await response.json())['match'])
            response=await self.client.get('/v1/matchmaking/transport',headers=auth)
            self.assertEqual(response.status,200,await response.text());descriptors.append(await response.json())
        self.assertEqual(matches[0]['settings'],matches[1]['settings'])
        self.assertEqual(matches[0]['game_id'],matches[1]['game_id'])
        raw=bytes.fromhex(matches[0]['settings'])
        self.assertEqual(matches[0]['settings_sha256'],hashlib.sha256(raw).hexdigest())
        self.assertEqual(len(raw),304)
        self.assertEqual(matches[0]['map'],'highplains')
        room=self.api.transport.rooms[descriptors[0]['id']]
        self.assertTrue(room.eos and room.eos_account and room.direct_udp)
        self.assertEqual(descriptors[0]['transport']['session']['map'],'highplains')
        self.assertEqual(len(room.players),4)
        report=self.report(descriptors);report[0x20:0x150]=raw;report[0x13]=1
        struct.pack_into('<I',report,0x64,struct.unpack_from('<i',raw,0x44)[0])
        for slot,(_,_,team) in enumerate(room.players):
            report[0x370+slot*0x94+0x88]=team;report[0xcb0+team*0x18]=1
        self.assertEqual(self.api.results.validate(room,report.hex())[0],report)
        # Unfamiliar objective names use their bound engine, never the old
        # unknown-name Slayer fallback. No fabricated result is committed.
        objective=next(g for e in self.api.entries for g in e.policy.games
                       if g.settings and struct.unpack_from('<i',bytes.fromhex(g.settings),0x44)[0]==1)
        room.proposal=replace(room.proposal,game=replace(objective,variant='Named objective'))
        report[0x20:0x150]=bytes.fromhex(objective.settings)
        struct.pack_into('<I',report,0x64,2)
        with self.assertRaises(Problem):self.api.results._validate_fixture(room,report.hex())
        struct.pack_into('<I',report,0x64,1)
        self.assertEqual(self.api.results._validate_fixture(room,report.hex())[0],report)

    async def lab_admission(self,*,couch):
        headers=[await self.session('one',couch=couch)]
        if not couch:headers.append(await self.session('two',couch=False))
        for auth in headers:
            await self.connect_qos(auth)
            response=await self.client.post('/v1/matchmaking/ticket',headers=auth,
                json=dict(playlist_id='lab-test',revision='lab-test-v1',request_id=str(uuid.uuid4())))
            self.assertEqual(response.status,201,await response.text())
        # Use the actual one-second Lab countdown. No backdated clock,
        # fake players, disabled connectivity or altered assignment.
        async with asyncio.timeout(6):
            while not self.api.core.proposals:
                for auth in headers:
                    response=await self.client.get('/v1/matchmaking/ticket',headers=auth)
                    self.assertEqual(response.status,200,await response.text())
                await asyncio.sleep(.02)
        for auth in headers:
            response=await self.client.get('/v1/matchmaking/ticket?ranks=1',headers=auth)
            match=(await response.json())['match']
            self.assertEqual(len(match['players']),2)
            self.assertEqual(sorted(p['team'] for p in match['players']),[0,1])
            self.assertTrue(all(p['level'] is None for p in match['players']))
            self.assertEqual(match['map'],'highplains')
            self.assertEqual(len(bytes.fromhex(match['settings'])),304)
            response=await self.client.get('/v1/matchmaking/transport',headers=auth)
            self.assertEqual(response.status,200,await response.text())
            descriptor=await response.json()
            self.assertEqual(len(descriptor['players']),2)
            self.assertEqual(len({p['peer'] for p in descriptor['players']}),1 if couch else 2)
            self.assertEqual(descriptor['transport']['session']['playlist'],'lab-test')
        db=self.api.directory.store.db
        self.assertEqual(db.execute('SELECT COUNT(*) FROM playlist_skill WHERE playlist=?',('lab-test',)).fetchone()[0],0)

    async def test_lab_two_solos_reach_eos_handoff_without_a_full_lobby(self):
        await self.lab_admission(couch=False)

    async def test_lab_primary_and_guest_reach_handoff_on_one_connection(self):
        await self.lab_admission(couch=True)


if __name__=='__main__':unittest.main()
