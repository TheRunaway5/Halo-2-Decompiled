"""Actual admitted appearance, including session-only guests; no name joins."""
import json
import unittest
from multiplayer.master_server.tests import test_matchmaking_http as baseline
from multiplayer.master_server.tests.test_couch_online import catalog


class MatchmakingAppearanceTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp=baseline.MatchmakingHttpTests.asyncSetUp
    catalog_fixture=staticmethod(catalog)
    enqueue=baseline.MatchmakingHttpTests.enqueue

    async def test_opt_in_identity_bound_profiles_and_cosmetic_renewal(self):
        primary='0a00000a0001020304';signed='020304050106070809';guest='03040506000a0b0c0d'
        stored='01020304000e0f1011';changed='040506070112131415'
        body=dict(screen_name='Same Name',maps=['lockout'],compatibility='halo2-native-queue-v12',language=0,
            appearance=primary,local_players=[dict(screen_name='Same Name',discord_token='three',appearance=signed),
                dict(screen_name='Same Name',appearance=guest),dict(screen_name='Same Name')])
        response=await self.client.post('/v1/matchmaking/session',headers={'Authorization':'Bearer one'},json=body)
        self.assertEqual(response.status,201,await response.text());token=(await response.json())['token'];a={'Authorization':'Bearer '+token}
        response=await self.client.post('/v1/matchmaking/session',headers={'Authorization':'Bearer two'},json=dict(
            screen_name='Same Name',maps=['lockout'],compatibility='halo2-native-queue-v12',language=0,
            local_players=[dict(screen_name='Same Name')]*3))
        self.assertEqual(response.status,201,await response.text());b={'Authorization':'Bearer '+(await response.json())['token']}
        sessions=list(self.api.sessions.values());first,second=sessions
        with self.api.directory.store.db:
            self.api.directory.store.db.execute('INSERT INTO player_customization VALUES(?,?,?,?,?)',(second.player,bytes.fromhex(stored),'',0,1))
        for h in (a,b):self.assertEqual((await self.enqueue(h,'casual')).status,201)
        legacy=await (await self.client.get('/v1/matchmaking/ticket',headers=a)).json()
        self.assertTrue(all(set(p)=={'name','team','self'} for p in legacy['match']['players']))
        value=await (await self.client.get('/v1/matchmaking/ticket?appearance=1&ranks=1&failure=1',headers=a)).json()
        self.assertIn('failure',value)
        rows=value['match']['players'];self.assertEqual(len(rows),8)
        self.assertEqual(sorted(p['appearance'] for p in rows if p['appearance'] is not None),sorted((primary,signed,guest,stored)))
        self.assertTrue(all(p['level'] is None for p in rows));self.assertNotIn('private-subject',json.dumps(value))
        old_ids=tuple(p.player for p in first.seats());old_ticket=self.api.ticket(first.player)
        body['renew_session']=token;body['local_players'][1]['appearance']=changed
        response=await self.client.post('/v1/matchmaking/session',headers={'Authorization':'Bearer one'},json=body)
        self.assertEqual(response.status,201,await response.text());self.assertEqual((await response.json())['token'],token)
        self.assertEqual(tuple(p.player for p in first.seats()),old_ids)
        # Rust snapshots preserve the whole immutable admission, including its
        # identity; Python object identity is no longer the queue authority.
        self.assertEqual(self.api.ticket(first.player),old_ticket)
        self.assertEqual(first.seats()[2].appearance,bytes.fromhex(changed));self.assertEqual(self.verified.count('three'),2)
        self.assertEqual(self.api.player_appearance(old_ids[2]),changed)
        self.assertIsNone(self.api.player_appearance(old_ids[3]))
        # A forged species or malformed encoding cannot mutate the accepted
        # session or manufacture a default emblem for the missing guest.
        for malformed in ('00'*4+'02'+'00'*4,'A0'*9,'00'*8):
            body['appearance']=malformed
            response=await self.client.post('/v1/matchmaking/session',headers={'Authorization':'Bearer one'},json=body)
            self.assertEqual(response.status,400)
            self.assertEqual(tuple(p.player for p in first.seats()),old_ids)
