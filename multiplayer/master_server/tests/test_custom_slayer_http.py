"""Action Sack admission keeps old builds out and preserves couch guest policy."""
from copy import deepcopy
import json
from pathlib import Path
import unittest
from . import test_couch_online as couch
from . import test_matchmaking_http as baseline
from . import test_match_transport as transport


class CustomSlayerHttpTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = baseline.MatchmakingHttpTests.asyncSetUp
    enqueue = couch.CouchOnlineTests.enqueue
    pair = transport.MatchTransportTests.pair

    @staticmethod
    def catalog_fixture():
        value=deepcopy(couch.catalog())
        for row in value['playlists']:
            row['required_compatibility']='halo2-native-queue-v10'
            row['team_size']=0
            row['games'][0]['variant']='Fiesta Slayer'
        return value

    async def session(self, owner='one', name='Chosen', *, compatibility='halo2-native-queue-v10'):
        response=await self.client.post('/v1/matchmaking/session',headers={'Authorization':'Bearer '+owner},
            json=dict(screen_name=(name+' '+owner if name=='Chosen' else name),maps=['lockout'],compatibility=compatibility,language=0,
                      local_players=[dict(screen_name='Guest '+str(i)) for i in range(2,5)]))
        self.assertEqual(response.status,201,await response.text())
        return {'Authorization':'Bearer '+(await response.json())['token']}

    async def test_old_native_cannot_enqueue_custom_rulesets(self):
        headers=await self.session(compatibility='halo2-native-queue-v9')
        response=await self.client.get('/v1/matchmaking/playlists',headers=headers)
        self.assertTrue(all(not r['eligible'] for r in (await response.json())['playlists']))
        response=await baseline.MatchmakingHttpTests.enqueue(self,headers,'casual')
        self.assertEqual(response.status,409)
        self.assertFalse(self.api.core.tickets)

    async def test_new_native_accepts_four_seats_with_transient_guests(self):
        headers=await self.session()
        response=await self.client.get('/v1/matchmaking/playlists',headers=headers)
        rows=(await response.json())['playlists']
        self.assertTrue(rows[1]['eligible']);self.assertFalse(rows[0]['eligible'])
        response=await baseline.MatchmakingHttpTests.enqueue(self,headers,'casual')
        self.assertEqual(response.status,201,await response.text())
        ticket=next(iter(self.api.core.tickets.values()))
        self.assertEqual(len(ticket.players),4)
        self.assertEqual(sum(p.guest for p in ticket.players),3)
        self.assertEqual(ticket.compatibility,'halo2-native-queue-v10')
        self.assertEqual(self.api.directory.store.db.execute('SELECT count(*) FROM players').fetchone()[0],1)

    async def test_release_catalog_preserves_labs_and_adds_only_supported_custom_modes(self):
        from multiplayer.master_server.matchmaking_http import load_catalog
        root=Path(__file__).resolve().parents[1]
        before=load_catalog(root/'examples/matchmaking-lab.json')
        after=load_catalog(root/'examples/matchmaking-action-sack.json')
        self.assertEqual(after[:-1],before)
        added=after[-1]
        self.assertEqual(added.name,'Action Sack');self.assertFalse(added.policy.ranked)
        self.assertEqual(added.required_compatibility,'halo2-native-queue-v10')
        self.assertEqual({g.variant for g in added.policy.games},{'Fiesta Slayer','Infection Slayer'})

    async def test_continuation_version_retains_custom_modes_and_guest_admission(self):
        headers=await self.session(compatibility='halo2-native-queue-v11')
        response=await self.client.get('/v1/matchmaking/playlists',headers=headers)
        self.assertTrue((await response.json())['playlists'][1]['eligible'])
        response=await baseline.MatchmakingHttpTests.enqueue(self,headers,'casual')
        self.assertEqual(response.status,201,await response.text())
        ticket=next(iter(self.api.core.tickets.values()))
        self.assertEqual(ticket.compatibility,'halo2-native-queue-v11')
        self.assertEqual(sum(p.guest for p in ticket.players),3)

    async def test_ffa_couch_descriptor_binds_mode_and_independent_memberships(self):
        _,descriptors,_,_,_=await self.pair()
        for row in descriptors:
            self.assertFalse(row['team_mode'])
            self.assertEqual(len(row['local_slots']),4)
            memberships={row['players'][slot]['team'] for slot in row['local_slots']}
            self.assertEqual(len(memberships),4)
