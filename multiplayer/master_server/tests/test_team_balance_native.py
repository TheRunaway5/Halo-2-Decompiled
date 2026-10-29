import ctypes
from dataclasses import replace
import json
from pathlib import Path
import unittest

from multiplayer.master_server.matchmaking import Coordinator, Player
from multiplayer.master_server.team_balance import native
from multiplayer.master_server.tests.test_matchmaking import policy


def multi_policy():
    return replace(policy('multi-team-native',ranked=False,radius=6),team_count=4,team_size=3,
                   minimum_players=12,maximum_players=12,maximum_party=12,party_balance=None)


def ticket(core,index,count):
    players=tuple(Player(f'p{index}-{seat}','Same Pilot',1,frozenset({'lockout'}),raw_rank=0)
                  for seat in range(count))
    return core.enqueue(f'a{index}','multi-team-native','test',players)


class OriginalNativeTeamBalance(unittest.TestCase):
    def test_actual_gathering_finalizer_uses_original_two_team_caller(self):
        from fractions import Fraction
        import random
        from types import SimpleNamespace
        from multiplayer.master_server.gathering_sessions import GatheringSessions,Gathering
        from multiplayer.master_server.matchmaking import SearchStage
        from multiplayer.master_server.search_lifecycle import GatheringConfiguration,GatheringClock
        rules=replace(policy('original-two-team',radius=49),party_balance=None,
            stages=(SearchStage(0,Fraction(1),0),),candidate_selection='retail-candidates-native-relay-v1',
            gathering=GatheringConfiguration((0,)*16,10000,'Declared finalization inputs'))
        core=Coordinator((rules,),clock=lambda:100.,rng=random.Random(2))
        ranks=(24,24,22,22,22,22,20,20)
        tickets=tuple(core.enqueue('party-'+str(i),rules.id,'synthetic-test',
            (Player('player-'+str(i),'P'+str(i),rank+1,frozenset(('lockout','ascension')),rank),))
            for i,rank in enumerate(ranks))
        owner=GatheringSessions(SimpleNamespace(core=core,search_qos=None),seed=1)
        gathering=Gathering('declared-gathering',tuple(t.id for t in tickets),100,
            tickets[0].players[0].id,0,rules.games[0],GatheringClock(100000,100000))
        gathering=owner._restore_fixture(gathering)
        proposal=owner.proposal(rules,gathering)
        self.assertIsNotNone(proposal)
        weights={p.id:p.raw_rank for t in tickets for p in t.players}
        self.assertEqual([[24,22,22,20]]*2,
            [sorted((weights[p] for p in team),reverse=True) for team in proposal.teams])

    def test_original_hopper_caller_fixture_through_service_abi(self):
        import struct
        fixture=json.loads((Path(__file__).resolve().parents[3]/'tests/fixtures/matchmaking_assignment_original.json').read_text())
        lib=native()
        for case in fixture['cases']:
            with self.subTest(case=case):
                header=bytearray(0x614)
                kind,teams,minimum,maximum,imbalance,flag,flag_min,flag_max=case['fields']
                struct.pack_into('<i',header,0,kind)
                struct.pack_into('<4i',header,0x5ec,teams,minimum,maximum,imbalance)
                header[0x604]=flag
                struct.pack_into('<2i',header,0x608,flag_min,flag_max)
                raw=ctypes.create_string_buffer(bytes(header))
                count=len(case['parties'])
                ids=(ctypes.c_int32*count)(*case['parties'])
                weights=(ctypes.c_int32*count)(*case['raw_weights'])
                output=(ctypes.c_int32*count)(*([777]*count))
                success=lib.h2_original_hopper_assignment(raw,len(header),count,ids,weights,output)
                self.assertEqual(bool(success),case['assignments'] is not None)
                self.assertEqual(list(output),case['assignments'] if success else [777]*count)

    def test_unchanged_tu15_fixture_through_actual_service_abi(self):
        fixture=json.loads((Path(__file__).resolve().parents[3]/'tests/fixtures/team_balance_tu15_original.json').read_text())
        self.assertEqual(fixture['original_sha256'],'21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd')
        self.assertEqual(len(fixture['cases']),256)
        lib=native()
        for case in fixture['cases']:
            with self.subTest(case=case):
                count=len(case['parties'])
                ids=(ctypes.c_int32*count)(*case['parties']);weights=(ctypes.c_int32*count)(*case['weights'])
                output=(ctypes.c_int32*count)(*([777]*count));spread=ctypes.c_int32(777)
                arguments=(case['teams'],case['maximum'],case['flagged'],case['minimum'],case['limit'],case['preserve'],count,ids)
                success=lib.h2_team_balance(*arguments,weights,case['optimize'],output)
                self.assertEqual(bool(success),case['assignments'] is not None)
                self.assertEqual(list(output),case['assignments'] if success else [777]*count)
                success=lib.h2_team_balance_count(*arguments,ctypes.byref(spread))
                self.assertEqual(bool(success),case['imbalance'] is not None)
                self.assertEqual(spread.value,case['imbalance'] if success else 777)
                self.assertEqual(bool(lib.h2_team_balance_can_add(*arguments,case['extra'],0)),case['can_add'])

    def test_four_whole_three_player_parties(self):
        rules=multi_policy();core=Coordinator((rules,),clock=lambda:0)
        group=tuple(ticket(core,i,3) for i in range(4))
        teams=core._teams(rules,group)
        self.assertEqual(tuple(map(len,teams)),(3,3,3,3))
        self.assertEqual({frozenset(p.id for p in t.players) for t in group},{frozenset(t) for t in teams})

    def test_casual_twelve_player_party_uses_original_split_fallback(self):
        rules=multi_policy();core=Coordinator((rules,),clock=lambda:0)
        group=(ticket(core,0,12),)
        teams=core._teams(rules,group)
        self.assertEqual(tuple(map(len,teams)),(3,3,3,3))
        self.assertEqual({p.id for p in group[0].players},{p for team in teams for p in team})

    def test_partial_gathering_keeps_open_slots_and_casual_split_fallback(self):
        from multiplayer.master_server.gathering_sessions import GatheringSessions
        rules=multi_policy();gathering=object.__new__(GatheringSessions)
        gathering.core=Coordinator((rules,),clock=lambda:0)
        core=gathering.core
        a,b,c=(ticket(core,i,3) for i in range(3))
        full,extra=ticket(core,30,12),ticket(core,40,1)
        self.assertTrue(gathering.fits(rules,(a,),b))
        self.assertTrue(gathering.fits(rules,(a,b),c))
        self.assertTrue(gathering.fits(rules,(),full))
        self.assertFalse(gathering.fits(rules,(full,),extra))

    def test_invalid_ffi_bounds_do_not_write_or_panic(self):
        lib=native();ids=(ctypes.c_int32*1)(0x7fffffff);weights=(ctypes.c_int32*1)(0);output=(ctypes.c_int32*1)(777)
        self.assertFalse(lib.h2_team_balance(4,3,False,0,0,True,1,ids,weights,True,output))
        self.assertEqual(output[0],777)
        self.assertFalse(lib.h2_team_balance_can_add(4,3,False,0,0,True,1,ids,1,0))
        self.assertFalse(lib.h2_team_balance(5,3,False,0,0,False,1,ids,weights,True,output))


class FourTeamHttpAdmission(unittest.IsolatedAsyncioTestCase):
    async def test_authenticated_three_seat_couches_reach_original_four_team_proposal(self):
        import tempfile
        import uuid
        from aiohttp.test_utils import TestClient,TestServer
        from multiplayer.master_server.service import Settings,create_app
        from multiplayer.master_server.protocol import Problem
        from multiplayer.master_server.player_identity import VerifiedOwner
        from multiplayer.master_server.tests.test_matchmaking_http import catalog
        with tempfile.TemporaryDirectory() as temp:
            fixture=catalog();fixture['playlists']=fixture['playlists'][1:]
            row=fixture['playlists'][0]
            row.update(id='multi-team',team_size=3,team_count=4,minimum_players=12,maximum_players=12,maximum_party=12,
                       rank_configuration='title-update-1.5-defaults',quickmatch=True)
            row['games'][0]['variant']='Team Slayer'
            config=Path(temp)/'playlists.json';config.write_text(json.dumps(fixture))
            verified=[]
            async def verify(session,token,application):
                if token not in ('one','two','three','four'):raise Problem(401,'Test identity denied')
                self.assertEqual(application,'1558522073272426519');verified.append(token)
                return VerifiedOwner('discord','private-'+token,token)
            app=create_app(Settings(state=Path(temp),public_url='http://127.0.0.1:8080',allow_http=True,
                matchmaking_catalog=str(config)),verify)
            async with TestClient(TestServer(app)) as client:
                headers=[]
                for owner in ('one','two','three','four'):
                    response=await client.post('/v1/matchmaking/session',headers={'Authorization':'Bearer '+owner},json=dict(
                        screen_name='Same Pilot',maps=['lockout'],compatibility='halo2-native-queue-v12',language=0,
                        local_players=[dict(screen_name='Same Guest'),dict(screen_name='Same Guest')]))
                    self.assertEqual(response.status,201,await response.text())
                    headers.append({'Authorization':'Bearer '+(await response.json())['token']})
                for h in headers:
                    response=await client.post('/v1/matchmaking/ticket',headers=h,json=dict(playlist_id='multi-team',revision='lab-1',request_id=str(uuid.uuid4())))
                    self.assertEqual(response.status,201,await response.text())
                api=next(iter(app.values())).matchmaking
                proposal=next(iter(api.core.proposals.values()))
                self.assertEqual(tuple(map(len,proposal.teams)),(3,3,3,3))
                self.assertEqual(len({p for team in proposal.teams for p in team}),12)
                for h in headers:
                    response=await client.get('/v1/matchmaking/ticket?ranks=1&progress=1',headers=h)
                    value=await response.json();self.assertEqual(value['state'],'checking_connections')
                    self.assertEqual(len(value['match']['players']),12)
                    self.assertEqual(sorted(p['team'] for p in value['match']['players']),[0]*3+[1]*3+[2]*3+[3]*3)
                    self.assertTrue(all(p['level'] is None for p in value['match']['players']))
                    self.assertNotIn('private-',json.dumps(value))
                self.assertEqual(verified,['one','two','three','four'])

    def test_four_team_catalog_requires_source_rating_owner_and_explicit_casual_teams(self):
        import tempfile
        from multiplayer.master_server.matchmaking_http import load_catalog
        from multiplayer.master_server.protocol import Problem
        from multiplayer.master_server.tests.test_matchmaking_http import catalog
        with tempfile.TemporaryDirectory() as temp:
            config=Path(temp)/'playlists.json'
            fixture=catalog();fixture['playlists']=fixture['playlists'][1:]
            row=fixture['playlists'][0];row.update(team_size=3,team_count=4,minimum_players=12,maximum_players=12,maximum_party=12)
            config.write_text(json.dumps(fixture))
            with self.assertRaises(Problem):load_catalog(config)
            row['rank_configuration']='title-update-1.5-defaults';config.write_text(json.dumps(fixture))
            self.assertEqual(load_catalog(config)[0].policy.team_count,4)
            row['ranked']=True;config.write_text(json.dumps(fixture))
            with self.assertRaises(Problem):load_catalog(config)
