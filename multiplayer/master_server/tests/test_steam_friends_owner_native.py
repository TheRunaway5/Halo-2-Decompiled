"""Actual account/HTTP/SQL controls with deterministic upstream Steam IO.

Python cache mutation is an internal authority control, not remote code access.
No real Steam grant or relationship is used by these tests.
"""
import asyncio
import json
import sqlite3
import time
import unittest
import uuid
from dataclasses import replace
from unittest.mock import patch
from . import test_steam_login as steam


class SteamFriendOwnerTests(unittest.IsolatedAsyncioTestCase):
    start=steam.SteamLoginTests.start
    signature=steam.SteamLoginTests.signature
    assertion=steam.SteamLoginTests.assertion
    complete=steam.SteamLoginTests.complete

    async def asyncSetUp(self):
        await steam.SteamLoginTests.asyncSetUp(self)
        self.directory.settings=replace(self.directory.settings,steam_key='1'*32)
        with patch.object(self.steam,'profile',return_value='Steam Fixture'):
            _,self.actor=await self.complete()
            _,self.target=await self.complete('76561197960287931')
        async def account(identity):
            response=await self.client.post('/v1/account/session',headers={'Authorization':'Bearer '+identity['token']},json={'screen_name':'Steam Fixture'})
            self.assertEqual(response.status,201,await response.text())
            return {'Authorization':'Bearer '+(await response.json())['token']}
        self.a,self.b=await account(self.actor),await account(self.target)
        self.subject=self.actor['identity']['id']
        self.target_subject=self.target['identity']['id']
        self.who=self.steam.db.execute("SELECT player FROM player_identities WHERE provider='steam' AND subject=?",(self.subject,)).fetchone()[0]
        self.upstream=[]
        self.on_reply=None
        self.private=False
        owner=self
        class Reply(steam.Response):
            async def __aenter__(self):
                callback=owner.on_reply
                owner.on_reply=None
                if callback:callback()
                return self
        def provider(url,*,params,allow_redirects):
            self.assertFalse(allow_redirects)
            self.assertEqual(params['key'],'1'*32)
            self.upstream.append((url,dict(params)))
            if 'GetFriendList' in url:
                return Reply(json.dumps({'friendslist':{'friends':[{'steamid':self.target_subject,'relationship':'friend'}]}}).encode(),403 if self.private else 200)
            return Reply(json.dumps({'response':{'players':[{'steamid':self.target_subject,'personaname':'Friend','personastate':1}]}}).encode())
        patched=patch.object(self.directory.session,'get',provider)
        patched.start();self.addCleanup(patched.stop)

    async def party(self):
        response=await self.client.post('/v1/party/commands',headers=self.a,json={'action':'create','request_id':str(uuid.uuid4())})
        self.assertEqual(response.status,200,await response.text())
        return {'action':'invite_steam','request_id':str(uuid.uuid4()),'version':(await response.json())['party']['version'],'steam_id':self.target_subject}

    def unlink(self):
        with self.steam.db:self.steam.db.execute("DELETE FROM player_identities WHERE player=? AND provider='steam'",(self.who,))

    async def test_get_revalidates_the_actual_native_account_after_provider_await(self):
        self.on_reply=lambda:self.directory.clans._sessions.revoke(self.who)
        response=await self.client.get('/v1/account/steam-friends',headers=self.a)
        self.assertEqual(response.status,401,await response.text())
        self.assertNotIn('friends',await response.json())

    async def test_provider_completion_revalidates_the_current_canonical_steam_link(self):
        self.on_reply=self.unlink
        response=await self.client.get('/v1/account/steam-friends',headers=self.a)
        self.assertEqual(response.status,403,await response.text())

    async def test_python_cache_alias_cannot_authorize_a_private_upstream_friend(self):
        body=await self.party()
        self.private=True
        self.directory.steam_friends.cache={self.subject:(time.monotonic()+60,{'state':'ready','friends':[{'id':self.target_subject,'name':'Forged Cache'}]})}
        response=await self.client.post('/v1/party/commands',headers=self.a,json=body)
        self.assertEqual(response.status,403,await response.text())
        self.assertEqual(self.steam.db.execute('SELECT count(*) FROM party_invites').fetchone()[0],0)

    async def test_invitation_rechecks_the_link_after_lookup_before_mutation(self):
        body=await self.party()
        actual=self.directory.steam_friends.lookup
        async def unlinked(*args,**kwargs):
            value=await actual(*args,**kwargs)
            self.unlink()
            return value
        with patch.object(self.directory.steam_friends,'lookup',side_effect=unlinked):
            response=await self.client.post('/v1/party/commands',headers=self.a,json=body)
        self.assertEqual(response.status,403,await response.text())
        self.assertEqual(self.steam.db.execute('SELECT count(*) FROM party_invites').fetchone()[0],0)

    async def test_returned_python_rows_cannot_replace_a_native_invitation_proof(self):
        body=await self.party()
        with patch.object(self.directory.steam_friends,'lookup',return_value={'state':'ready','friends':[{'id':self.target_subject,'name':'Forged'}]}):
            response=await self.client.post('/v1/party/commands',headers=self.a,json=body)
        self.assertEqual(response.status,400,await response.text())
        self.assertEqual(self.steam.db.execute('SELECT count(*) FROM party_invites').fetchone()[0],0)

    async def test_provider_await_has_no_open_sql_transaction_and_outer_transactions_cannot_publish(self):
        transactions=[]
        self.on_reply=lambda:transactions.append(self.steam.db.in_transaction)
        response=await self.client.get('/v1/account/steam-friends',headers=self.a)
        self.assertEqual(response.status,200,await response.text())
        self.assertEqual(transactions,[False])
        auth=self.directory.clans._sessions.call('auth',header=self.a['Authorization'],rate=False)
        from multiplayer.master_server.native_steam_friends import run
        from multiplayer.master_server.protocol import Problem
        db=self.steam.db
        db.execute('BEGIN')
        try:
            with self.assertRaises(Problem) as denied:await run(self.directory.steam_friends,self.who,auth['key'],mode='view')
            self.assertEqual(denied.exception.status,409)
            self.assertTrue(db.in_transaction)
        finally:db.rollback()

    async def test_credential_change_while_provider_replies_cannot_publish_old_data(self):
        self.on_reply=lambda:setattr(self.directory,'settings',replace(self.directory.settings,steam_key='2'*32))
        response=await self.client.get('/v1/account/steam-friends',headers=self.a)
        self.assertEqual(response.status,409,await response.text())
        stats=self.directory.steam_friends._cache.stats()
        self.assertEqual((stats['cached'],stats['grants'],stats['active']),(0,0,False))

    async def test_projection_commit_failure_leaves_no_shared_cache_and_retry_is_fresh(self):
        from multiplayer.master_server import player_presence
        actual=player_presence.observed
        commits=[]
        def denied(action,operation,column,database,trigger):
            if action==sqlite3.SQLITE_TRANSACTION and operation=='COMMIT':
                commits.append(True)
                return sqlite3.SQLITE_DENY
            return sqlite3.SQLITE_OK
        def armed(directory):
            result=actual(directory)
            self.steam.db.set_authorizer(denied)
            return result
        try:
            with patch.object(player_presence,'observed',side_effect=armed):
                response=await self.client.get('/v1/account/steam-friends',headers=self.a)
            self.assertEqual(response.status,500)
        finally:self.steam.db.set_authorizer(None)
        self.assertTrue(commits)
        stats=self.directory.steam_friends._cache.stats()
        self.assertEqual((stats['cached'],stats['active'],stats['waiting']),(0,False,0))
        calls=len(self.upstream)
        response=await self.client.get('/v1/account/steam-friends',headers=self.a)
        self.assertEqual(response.status,200,await response.text())
        self.assertEqual(len(self.upstream),calls+2)

    async def test_invitation_proof_is_command_bound_commit_fenced_and_replay_safe(self):
        from multiplayer.master_server.native_party_directory import run
        from multiplayer.master_server.protocol import Problem
        body=await self.party()
        api=self.directory.steam_friends
        auth=self.directory.clans._sessions.call('auth',header=self.a['Authorization'],rate=False)
        claim=await api.lookup(self.who,account_key=auth['key'],body=body)
        self.assertIsInstance(claim,str)
        self.assertEqual(api._cache.stats()['grants'],1)
        with self.assertRaises(Problem) as denied:
            run(self.directory.parties,'command',self.who,account_key=auth['key'],body={**body,'request_id':str(uuid.uuid4())},steam=claim)
        self.assertEqual(denied.exception.status,403)
        self.steam.db.execute("CREATE TEMP TRIGGER reject_steam_invite BEFORE INSERT ON party_invites BEGIN SELECT RAISE(ABORT,'declared Steam invite failure'); END")
        try:
            with self.assertRaises(sqlite3.IntegrityError):run(self.directory.parties,'command',self.who,account_key=auth['key'],body=body,steam=claim)
        finally:self.steam.db.execute('DROP TRIGGER reject_steam_invite')
        self.assertEqual(self.steam.db.execute('SELECT count(*) FROM party_invites').fetchone()[0],0)
        self.assertEqual((api._cache.stats()['grants'],api._cache.stats()['pending']),(1,0))
        run(self.directory.parties,'command',self.who,account_key=auth['key'],body=body,steam=claim)
        self.assertEqual((api._cache.stats()['grants'],api._cache.stats()['pending']),(0,0))
        run(self.directory.parties,'command',self.who,account_key=auth['key'],body=body,steam=claim)
        self.assertEqual(self.steam.db.execute('SELECT count(*) FROM party_invites').fetchone()[0],1)

    async def test_cancelled_active_lookup_releases_the_native_fifo_for_a_waiting_request(self):
        body=await self.party()
        api=self.directory.steam_friends
        auth=self.directory.clans._sessions.call('auth',header=self.a['Authorization'],rate=False)
        entered=asyncio.Event();release=asyncio.Event()
        class Paused(steam.Response):
            async def __aenter__(self):entered.set();await release.wait();return self
        calls=[]
        def provider(url,*,params,allow_redirects):
            calls.append(url)
            if len(calls)==1:return Paused(b'{"friendslist":{"friends":[]}}')
            return steam.Response(b'{"friendslist":{"friends":[]}}')
        with patch.object(self.directory.session,'get',provider):
            active=asyncio.create_task(api.lookup(self.who,account_key=auth['key'],body=body))
            try:
                await asyncio.wait_for(entered.wait(),2)
                waiting=asyncio.create_task(self.client.get('/v1/account/steam-friends',headers=self.a))
                try:
                    for _ in range(100):
                        if api._cache.stats()['waiting']==1:break
                        await asyncio.sleep(.01)
                    self.assertEqual(api._cache.stats()['waiting'],1)
                    active.cancel()
                    with self.assertRaises(asyncio.CancelledError):await active
                    response=await asyncio.wait_for(waiting,2)
                    self.assertEqual(response.status,200,await response.text())
                    self.assertEqual(len(calls),2)
                finally:
                    if not waiting.done():waiting.cancel()
                    await asyncio.gather(waiting,return_exceptions=True)
            finally:
                release.set()
                if not active.done():active.cancel()
                await asyncio.gather(active,return_exceptions=True)
        self.assertEqual((api._cache.stats()['active'],api._cache.stats()['waiting']),(False,0))

    async def test_first_thousand_friends_use_ten_bounded_summary_requests_and_full_sized_responses(self):
        identities=[self.target_subject]+[str(76561197960300000+i) for i in range(1000)]
        batches=[]
        def provider(url,*,params,allow_redirects):
            if 'GetFriendList' in url:return steam.Response(json.dumps({'friendslist':{'friends':[{'steamid':sid,'relationship':'friend'} for sid in reversed(identities)]}}).encode())
            selected=params['steamids'].split(',');batches.append(selected)
            return steam.Response(json.dumps({'response':{'players':[{'steamid':sid,'personaname':'Name '+sid,'personastate':1,'ignored_metadata':'x'*1024} for sid in selected]}}).encode())
        with patch.object(self.directory.session,'get',provider):
            response=await self.client.get('/v1/account/steam-friends',headers=self.a)
        self.assertEqual(response.status,200,await response.text())
        value=await response.json()
        self.assertTrue(value['truncated'])
        self.assertEqual(len(value['friends']),1000)
        self.assertEqual([len(b) for b in batches],[100]*10)
        self.assertEqual(sum(batches,[]),sorted(identities)[:1000])
        self.assertEqual(value['friends'][0]['id'],self.target_subject)
        self.assertTrue(value['friends'][0]['halo_online'])
        self.assertNotIn('ignored_metadata',json.dumps(value))

    async def test_duplicate_json_and_unrequested_profile_ids_cannot_supply_authority(self):
        for raw in (b'{"friendslist":{"friends":[],"fri\\u0065nds":[]}}',b'{"friendslist":{"friends":null}}'):
            self.directory.steam_friends._cache.clear()
            with patch.object(self.directory.session,'get',return_value=steam.Response(raw)):
                response=await self.client.get('/v1/account/steam-friends',headers=self.a)
            self.assertEqual(response.status,502,await response.text())
        # A friend from the next100-ID batch cannot poison that future row.
        identities=[self.target_subject]+[str(76561197960300000+i) for i in range(100)]
        last=identities[-1]
        def provider(url,*,params,allow_redirects):
            if 'GetFriendList' in url:return steam.Response(json.dumps({'friendslist':{'friends':[{'steamid':sid,'relationship':'friend'} for sid in identities]}}).encode())
            name='Requested Profile' if last in params['steamids'].split(',') else 'Unrequested Profile'
            return steam.Response(json.dumps({'response':{'players':[{'steamid':last,'personaname':name,'personastate':1}]}}).encode())
        with patch.object(self.directory.session,'get',provider):
            response=await self.client.get('/v1/account/steam-friends',headers=self.a)
        self.assertEqual(response.status,200,await response.text())
        self.assertEqual((await response.json())['friends'],[{'id':last,'name':'Requested Profile','registered':False,'halo_online':False}])
