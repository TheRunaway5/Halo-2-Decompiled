import sqlite3
import unittest
from unittest.mock import patch
from multiplayer.master_server.player_identity import initialize, player
from multiplayer.master_server.player_statistics import enqueue
from . import test_carnage_history as history


class ProfileMigrationTests(unittest.TestCase):
    def test_joined_is_server_issued_stable_and_legacy_unknown(self):
        db=sqlite3.connect(":memory:")
        try:
            db.executescript("CREATE TABLE players(id TEXT PRIMARY KEY,provider TEXT,subject TEXT);"
                "CREATE TABLE player_profiles(player TEXT PRIMARY KEY,screen_name TEXT,discord_username TEXT NOT NULL DEFAULT '');"
                "INSERT INTO players VALUES('old','discord','old');INSERT INTO player_profiles VALUES('old','Old','old');")
            initialize(db)
            self.assertIsNone(db.execute("SELECT joined_at FROM player_profiles WHERE player='old'").fetchone()[0])
            who=player(db,('discord','new'),'Pilot','pilot',now=1700000000)
            self.assertEqual(player(db,('discord','new'),'Changed','pilot',now=1900000000),who)
            self.assertEqual(db.execute("SELECT joined_at FROM player_profiles WHERE player=?",(who,)).fetchone()[0],1700000000)
        finally:db.close()


class PlayerProfileTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp=history.CarnageHistoryTests.asyncSetUp
    catalog_fixture=staticmethod(history.CarnageHistoryTests.catalog_fixture)
    session=history.CarnageHistoryTests.session
    enqueue=history.CarnageHistoryTests.enqueue
    pair=history.CarnageHistoryTests.pair
    report=history.CarnageHistoryTests.report
    completed=history.CarnageHistoryTests.completed

    async def login(self,token='one'):
        response=await self.client.post('/v1/account/session',headers={'Authorization':'Bearer '+token},json={'screen_name':'Pilot '+token})
        self.assertEqual(response.status,201)
        return {'Authorization':'Bearer '+(await response.json())['token']}

    async def test_current_appearance_tag_joined_and_history_are_separate(self):
        room,raw=await self.completed()
        headers=await self.login()
        first=await (await self.client.get('/v1/account/profile',headers=headers)).json()
        data={'appearance':'ff020304013f1f81a5','service_tag':'A117'}
        response=await self.client.put('/v1/account/profile',headers=headers,json=data)
        self.assertEqual(response.status,200)
        profile=await response.json()
        self.assertEqual(profile['appearance']['species'],'elite')
        self.assertEqual(profile['appearance']['colors'],[-1,2,3,4])
        self.assertEqual(profile['appearance']['emblem'],dict(foreground=63,background=31,flags=129,reserved=165))
        self.assertEqual(profile['service_tag'],'A117')
        self.assertEqual(profile['joined_at'],first['joined_at'])
        self.assertEqual(profile['statistics']['kills'],7)
        self.assertEqual(profile['statistics']['shots'],11)
        self.assertTrue(profile['statistics']['accuracy_complete'])
        unchanged=await (await self.client.put('/v1/account/profile',headers=headers,json=data)).json()
        self.assertEqual(unchanged['revision'],profile['revision'])
        public=(await (await self.client.get('/v1/history/players?discord_username=one')).json())['players'][0]['id']
        public_profile=await (await self.client.get('/v1/history/players/'+public+'/profile')).json()
        self.assertEqual(public_profile['appearance'],profile['appearance'])
        db=self.api.directory.store.db
        self.assertEqual(bytes(db.execute('SELECT appearance FROM carnage_players WHERE match=? AND player=?',(room.id,room.players[0][0])).fetchone()[0]),raw[0x3bc:0x3cc])
        self.assertNotIn(room.players[0][0],str(public_profile))

    async def test_write_requires_owner_session_and_rejects_server_fields(self):
        headers=await self.login()
        good={'appearance':'0a00000a0000000000','service_tag':'S001'}
        self.assertEqual((await self.client.put('/v1/account/profile',json=good)).status,401)
        for field,value in [('joined_at',0),('player','someone'),('statistics',{'kills':999}),('puid','0'*32)]:
            self.assertEqual((await self.client.put('/v1/account/profile',headers=headers,json=good|{field:value})).status,400)
        for change in [{'service_tag':'TOOLONG'},{'service_tag':'a117'},{'appearance':'0a00000a0200000000'},{'appearance':'00'*16}]:
            self.assertEqual((await self.client.put('/v1/account/profile',headers=headers,json=good|change)).status,400)
        self.assertEqual((await self.client.put('/v1/account/profile',headers=headers,json=good)).status,200)
        other=await self.login('two')
        self.assertIsNone((await (await self.client.get('/v1/account/profile',headers=other)).json())['appearance'])

    async def test_profile_heartbeats_reuse_totals_until_an_accepted_revision_changes(self):
        room,_=await self.completed()
        headers=await self.login()
        db=self.api.directory.store.db
        queries=[]
        db.set_trace_callback(queries.append)
        self.addCleanup(db.set_trace_callback,None)
        first=await (await self.client.get('/v1/account/profile',headers=headers)).json()
        queries.clear()
        again=await (await self.client.get('/v1/account/profile',headers=headers)).json()
        self.assertEqual(again['statistics'],first['statistics'])
        self.assertFalse(any('FROM match_result_players' in q or 'FROM carnage_weapon_stats' in q for q in queries),
                         'Unchanged profile heartbeats must not rescan lifetime match/weapon history')
        with db:
            db.execute('UPDATE match_result_players SET kills=kills+1 WHERE player=?',(room.players[0][0],))
            enqueue(db,room.players[0][0])
        queries.clear()
        newer=await (await self.client.get('/v1/account/profile',headers=headers)).json()
        self.assertEqual(newer['statistics']['kills'],first['statistics']['kills']+1)
        self.assertTrue(any('FROM match_result_players' in q for q in queries))

    async def test_cached_totals_expire_without_an_outbox_revision_change(self):
        room,_=await self.completed()
        headers=await self.login()
        db=self.api.directory.store.db
        self.api.directory.profiles._statistics.clear()
        with patch('multiplayer.master_server.player_profile.monotonic',return_value=100):
            first=await (await self.client.get('/v1/account/profile',headers=headers)).json()
        # This deliberately models maintenance outside the accepted-result path.
        with db:db.execute('UPDATE match_result_players SET kills=kills+1 WHERE player=?',(room.players[0][0],))
        with patch('multiplayer.master_server.player_profile.monotonic',return_value=401):
            refreshed=await (await self.client.get('/v1/account/profile',headers=headers)).json()
        self.assertEqual(refreshed['statistics']['kills'],first['statistics']['kills']+1)

    async def test_career_accuracy_queries_search_only_the_selected_player(self):
        db=self.api.directory.store.db
        queries=["SELECT field,sum(value) FROM carnage_weapon_stats WHERE player=? "
                 "AND weapon BETWEEN 5 AND 38 AND field IN (4,5) GROUP BY field",
                 "SELECT count(*) FROM carnage_players WHERE player=?"]
        for query in queries:
            plan=' '.join(row[3] for row in db.execute('EXPLAIN QUERY PLAN '+query,('fixture-player',)))
            self.assertIn('SEARCH',plan,'Career queries must avoid scans of every player: '+plan)
            self.assertNotIn('SCAN',plan)
