"""Native profile authorization/cache/transaction ownership via actual HTTP."""
import asyncio
import json
import unittest
import sqlite3
from unittest.mock import patch
from . import test_player_profile as profiles


class ProfileOwnerTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp=profiles.PlayerProfileTests.asyncSetUp
    catalog_fixture=staticmethod(profiles.PlayerProfileTests.catalog_fixture)
    session=profiles.PlayerProfileTests.session
    enqueue=profiles.PlayerProfileTests.enqueue
    pair=profiles.PlayerProfileTests.pair
    report=profiles.PlayerProfileTests.report
    completed=profiles.PlayerProfileTests.completed
    login=profiles.PlayerProfileTests.login

    async def test_signout_while_profile_body_uploads_cannot_publish_customization(self):
        headers=await self.login()
        db=self.api.directory.store.db
        before=list(db.execute('SELECT player,revision FROM eos_profile_outbox ORDER BY player'))
        authenticated,release=asyncio.Event(),asyncio.Event()
        actual=self.api.directory.clans.session
        def session(request,**kwargs):
            result=actual(request,**kwargs)
            if request.method=='PUT' and request.path=='/v1/account/profile':authenticated.set()
            return result
        async def upload():
            yield b'{"appearance":"0a00000a0000000000",'
            await release.wait()
            yield b'"service_tag":"S001"}'
        with patch.object(self.api.directory.clans,'session',side_effect=session):
            task=asyncio.create_task(self.client.put('/v1/account/profile',headers=headers|{'Content-Type':'application/json'},data=upload()))
            try:
                await asyncio.wait_for(authenticated.wait(),2)
                signed_out=await self.client.delete('/v1/account/session',headers=headers)
                self.assertEqual(signed_out.status,200,await signed_out.text())
                release.set()
                response=await asyncio.wait_for(task,2)
                self.assertEqual(response.status,401,await response.text())
            finally:
                release.set()
                if not task.done():
                    task.cancel()
                    try:await task
                    except asyncio.CancelledError:pass
        self.assertEqual(db.execute('SELECT count(*) FROM player_customization').fetchone()[0],0)
        self.assertEqual(list(db.execute('SELECT player,revision FROM eos_profile_outbox ORDER BY player')),before)

    async def test_rolled_back_outer_statistics_never_enter_the_shared_cache(self):
        room,_=await self.completed()
        profiles=self.api.directory.profiles
        db=self.api.directory.store.db
        who=next(p for p,_,_ in room.players if p not in room.guests)
        profiles.cache.clear()
        db.execute('BEGIN')
        try:
            db.execute('UPDATE match_result_players SET kills=77 WHERE player=?',(who,))
            self.assertEqual(profiles.statistics(who)['kills'],77)
            self.assertTrue(db.in_transaction)
            self.assertEqual(profiles.cache.stats(),{'cached':0,'pending':0})
        finally:db.rollback()
        self.assertEqual(profiles.statistics(who)['kills'],7)
        self.assertEqual(profiles.cache.stats(),{'cached':1,'pending':0})

    async def test_sql_commit_failure_aborts_staged_cache_customization_and_outbox_together(self):
        await self.completed()
        headers=await self.login()
        profiles=self.api.directory.profiles
        db=self.api.directory.store.db
        first=await (await self.client.get('/v1/account/profile',headers=headers)).json()
        before=list(db.execute('SELECT player,revision FROM eos_profile_outbox ORDER BY player'))
        floor=db.execute('SELECT value FROM clock_floor WHERE id=1').fetchone()[0]
        with db:
            db.executescript("""CREATE TABLE profile_commit_failure(player TEXT REFERENCES players(id) DEFERRABLE INITIALLY DEFERRED);
              CREATE TRIGGER delayed_profile_failure AFTER INSERT ON player_customization
              BEGIN INSERT INTO profile_commit_failure VALUES('declared-absent-fixture-player'); END;""")
        commits=[]
        def observe(action,a,b,c,d):
            if action==sqlite3.SQLITE_TRANSACTION and a=='COMMIT':commits.append(profiles.cache.stats())
            return sqlite3.SQLITE_OK
        db.set_authorizer(observe)
        try:
            with patch.object(profiles,'wall',return_value=floor+100):
                response=await self.client.put('/v1/account/profile',headers=headers,
                    json={'appearance':'ff020304013f1f81a5','service_tag':'A117'})
            self.assertEqual(response.status,500,await response.text())
        finally:db.set_authorizer(None)
        self.assertTrue(any(row['pending']==1 for row in commits),'No actual staged cache existed at the failed SQL commit')
        self.assertEqual(profiles.cache.stats(),{'cached':0,'pending':0})
        self.assertEqual(db.execute('SELECT count(*) FROM player_customization').fetchone()[0],0)
        self.assertEqual(db.execute('SELECT count(*) FROM profile_commit_failure').fetchone()[0],0)
        self.assertEqual(list(db.execute('SELECT player,revision FROM eos_profile_outbox ORDER BY player')),before)
        self.assertEqual(db.execute('SELECT value FROM clock_floor WHERE id=1').fetchone()[0],floor)
        with db:db.execute('DROP TRIGGER delayed_profile_failure')
        result=await (await self.client.put('/v1/account/profile',headers=headers,
            json={'appearance':'ff020304013f1f81a5','service_tag':'A117'})).json()
        self.assertEqual(result['revision'],1)
        self.assertEqual(result['statistics'],first['statistics'])

    async def test_python_cache_aliases_cannot_install_career_statistics(self):
        room,_=await self.completed()
        profiles=self.api.directory.profiles
        who=next(p for p,_,_ in room.players if p not in room.guests)
        profiles._statistics={who:(None,profiles.clock()+99999,{'kills':999999})}
        self.assertEqual(profiles.statistics(who)['kills'],7)
        from multiplayer.master_server.protocol import Problem
        with self.assertRaises(Problem) as denied:profiles.cache.call('install')
        self.assertEqual(denied.exception.status,400)
