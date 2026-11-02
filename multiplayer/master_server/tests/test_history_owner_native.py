"""Public-history ownership through actual HTTP and the native SQL cursor.

The projection control is an internal adapter-authority check, not a claim
that an unauthenticated client can replace Python methods remotely.
"""
import json
import base64
import uuid
import unittest
from unittest.mock import patch
from . import test_carnage_history as history


class PublicHistoryOwnerTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp=history.CarnageHistoryTests.asyncSetUp
    catalog_fixture=staticmethod(history.CarnageHistoryTests.catalog_fixture)
    session=history.CarnageHistoryTests.session
    enqueue=history.CarnageHistoryTests.enqueue
    pair=history.CarnageHistoryTests.pair
    report=history.CarnageHistoryTests.report
    completed=history.CarnageHistoryTests.completed

    async def test_python_profile_projection_cannot_publish_private_history_fields(self):
        room,_=await self.completed()
        db=self.api.directory.store.db
        private=next(p for p,_,_ in room.players if p not in room.guests)
        public=db.execute('SELECT public_id FROM history_players WHERE player=?',(private,)).fetchone()[0]
        expected=db.execute('SELECT screen_name FROM player_profiles WHERE player=?',(private,)).fetchone()[0]
        projection=dict(screen_name='forged adapter name',private_identity=private,
                        statistics=dict(kills=999999))
        with patch.object(self.api.directory.profiles,'view',return_value=projection):
            response=await self.client.get('/v1/history/players/'+public+'/matches')
            self.assertEqual(response.status,200,await response.text())
            body=await response.json()
            self.assertNotIn('private_identity',body['player'])
            self.assertEqual(body['player']['screen_name'],expected)
            self.assertEqual(body['player']['statistics']['kills'],7)
            profile=await (await self.client.get('/v1/history/players/'+public+'/profile')).json()
            self.assertNotIn('private_identity',profile)
            self.assertEqual(profile['screen_name'],expected)
        self.assertNotIn(private,json.dumps(body))

    async def test_complete_persisted_weapon_bank_uses_bounded_pages_without_truncation(self):
        room,_=await self.completed()
        db=self.api.directory.store.db
        row=db.execute('SELECT player,slot,screen_name,discord_username_at_match FROM carnage_player_summaries WHERE match=? ORDER BY slot LIMIT 1',(room.id,)).fetchone()
        who,slot,name,username=row
        # A declared older detailed-row storage fixture. This does not upgrade
        # the current verified-summary publisher into full-detail authority.
        with db:
            db.execute('INSERT INTO carnage_players VALUES(?,?,?,?,?,?)',(room.id,who,slot,name,username,bytes(range(16))))
            db.executemany('INSERT INTO carnage_weapon_stats VALUES(?,?,?,?,?)',
                [(room.id,who,weapon,field,weapon*8+field+1) for weapon in range(45) for field in range(8)])
        public=db.execute('SELECT public_id FROM carnage_reports WHERE match=?',(room.id,)).fetchone()[0]
        statements=[];db.set_trace_callback(statements.append)
        try:
            response=await self.client.get('/v1/history/matches/'+public)
            self.assertEqual(response.status,200,await response.text())
            body=await response.json()
        finally:db.set_trace_callback(None)
        player=next(p for p in body['players'] if p['slot']==slot)
        self.assertEqual(player['weapons'],[[w,f,w*8+f+1] for w in range(45) for f in range(8)])
        self.assertEqual(player['appearance'],bytes(range(16)).hex())
        pages=[s for s in statements if 'FROM carnage_weapon_stats' in s and who in s]
        self.assertEqual(len(pages),3)
        self.assertTrue(all('LIMIT 128 OFFSET' in s for s in pages))
        self.assertNotIn(who,json.dumps(body))

    async def test_public_reads_validate_cursor_types_duplicates_and_never_write(self):
        await self.completed()
        db=self.api.directory.store.db
        public=db.execute('SELECT public_id FROM history_players ORDER BY public_id LIMIT 1').fetchone()[0]
        actual_read=self.api.directory.history.read
        def read_without_writes(*args,**kwargs):
            # The cursor is synchronous. Scope the witness to that actual
            # read so background maintenance/outbox work cannot alter it.
            before=db.total_changes
            try:return actual_read(*args,**kwargs)
            finally:self.assertEqual(db.total_changes,before)
        def cursor(value):return base64.urlsafe_b64encode(json.dumps(value).encode()).decode().rstrip('=')
        with patch.object(self.api.directory.history,'read',side_effect=read_without_writes):
            for value in [[True,str(uuid.uuid4())],[-1,str(uuid.uuid4())],[1.0,str(uuid.uuid4())],
                          [9007199254740992,str(uuid.uuid4())],[0,'not-an-id'],{},[0,str(uuid.uuid4()),0]]:
                response=await self.client.get('/v1/history/players/'+public+'/matches',params={'before':cursor(value)})
                self.assertEqual(response.status,400,await response.text())
            for query in ['limit=1&limit=2','before=abc&before=def','extra=1']:
                self.assertEqual((await self.client.get('/v1/history/players/'+public+'/matches?'+query)).status,400)
            self.assertEqual((await self.client.get('/v1/history/players/'+public+'/profile?extra=1')).status,400)
            self.assertEqual((await self.client.get('/v1/history/players/'+public+'/matches')).status,200)

    async def test_history_read_preserves_callers_uncommitted_transaction(self):
        await self.completed()
        db=self.api.directory.store.db
        who,public=db.execute('SELECT player,public_id FROM history_players ORDER BY public_id LIMIT 1').fetchone()
        old=db.execute('SELECT screen_name FROM player_profiles WHERE player=?',(who,)).fetchone()[0]
        db.execute('BEGIN')
        try:
            db.execute('UPDATE player_profiles SET screen_name=? WHERE player=?',('Pending Name',who))
            response=await self.client.get('/v1/history/players/'+public+'/matches')
            self.assertEqual(response.status,200,await response.text())
            self.assertEqual((await response.json())['player']['screen_name'],'Pending Name')
            self.assertTrue(db.in_transaction,'Public read committed unrelated in-flight writes')
        finally:db.rollback()
        self.assertEqual(db.execute('SELECT screen_name FROM player_profiles WHERE player=?',(who,)).fetchone()[0],old)
