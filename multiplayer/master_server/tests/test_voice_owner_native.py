"""Real HTTP/SQLite/socket controls for the native private voice owners.

Adapter mutation and direct SQL churn are internal ownership controls, not a
claim that a remote peer can edit the server's Python objects or database.
"""
import asyncio
from contextlib import closing
import sqlite3
import time
import unittest
from unittest.mock import patch
from aiohttp import WSMsgType
from . import test_clans_http as clans
from .test_voice_blocks import numeric_owners


class VoicePreferenceOwnerTests(unittest.IsolatedAsyncioTestCase):
    start = clans.ClansHttpTests.start
    login = clans.ClansHttpTests.login
    asyncSetUp = clans.ClansHttpTests.asyncSetUp

    def actors(self):
        directory = self.api.directory
        owners = numeric_owners(directory)
        actors = dict((subject, player) for player, subject in directory.store.db.execute('SELECT id,subject FROM players'))
        return directory, owners, actors[owners['one']], actors[owners['two']]

    async def replace(self, blocked):
        response = await self.client.put('/v1/account/voice-blocks', headers={'Authorization':'Bearer one'}, json={'blocked':blocked})
        self.assertEqual(response.status,200,await response.text())

    async def test_adapter_alias_cannot_clear_a_persisted_voice_preference(self):
        directory, owners, a, b = self.actors()
        await self.replace([owners['two']])
        directory.voice_blocks.blocks = {}
        directory.voice_blocks.subjects = {}
        self.assertFalse(directory.voice_blocks.allowed(a,b))
        self.assertFalse(directory.voice_blocks.allowed(b,a))

    async def test_another_connection_invalidates_current_identity_and_preference_reads(self):
        directory, owners, a, b = self.actors()
        self.assertTrue(directory.voice_blocks.allowed(a,b))
        with closing(sqlite3.connect(directory.settings.state/'registry.sqlite3')) as writer, writer:
            writer.execute('INSERT INTO voice_blocks VALUES(?,?)',(a,owners['two']))
        self.assertFalse(directory.voice_blocks.allowed(a,b))
        with closing(sqlite3.connect(directory.settings.state/'registry.sqlite3')) as writer, writer:
            writer.execute('UPDATE players SET subject=? WHERE id=?',('999999999999999999',b))
        self.assertTrue(directory.voice_blocks.allowed(a,b))
        with closing(sqlite3.connect(directory.settings.state/'registry.sqlite3')) as writer, writer:
            writer.execute('UPDATE players SET subject=? WHERE id=?',(owners['two'],b))
        self.assertFalse(directory.voice_blocks.allowed(a,b))

    async def test_preference_update_preserves_an_outer_transaction_and_rollback(self):
        directory, owners, a, b = self.actors()
        db = directory.store.db
        db.execute('CREATE TEMP TABLE voice_outer_marker(value INTEGER)')
        db.execute('BEGIN')
        db.execute('INSERT INTO voice_outer_marker VALUES(117)')
        try:
            await self.replace([owners['two']])
            self.assertTrue(db.in_transaction)
            self.assertFalse(directory.voice_blocks.allowed(a,b))
        finally:db.rollback()
        self.assertEqual(db.execute('SELECT count(*) FROM voice_outer_marker').fetchone()[0],0)
        self.assertTrue(directory.voice_blocks.allowed(a,b))


class ClanVoiceOwnerTests(unittest.IsolatedAsyncioTestCase):
    start = clans.ClansHttpTests.start
    login = clans.ClansHttpTests.login
    asyncSetUp = clans.ClansHttpTests.asyncSetUp
    command = clans.ClansHttpTests.command
    view = clans.ClansHttpTests.view
    join = clans.ClansHttpTests.join
    make_clan = clans.ClansHttpTests.make_clan

    async def test_leave_rejoin_incarnation_retires_an_already_connected_clan_socket(self):
        await self.make_clan()
        await self.join(self.b,'pilot_two')
        a = await self.client.ws_connect('/v1/clans/voice',headers=self.a)
        b = await self.client.ws_connect('/v1/clans/voice',headers=self.b)
        self.addAsyncCleanup(a.close);self.addAsyncCleanup(b.close)
        db = self.api.db
        who = db.execute("SELECT player FROM player_profiles WHERE discord_username='pilot_two'").fetchone()[0]
        row = db.execute('SELECT * FROM clan_members WHERE player=?',(who,)).fetchone()
        # No event-loop interleave: the old channel must observe the removed
        # membership even when a new membership has the same clan and role.
        with db:
            db.execute('DELETE FROM clan_members WHERE player=?',(who,))
            db.execute('INSERT INTO clan_members VALUES(?,?,?,?)',tuple(row))
        await a.send_bytes(b'\x01\0\0\0\x01a')
        self.assertEqual((await b.receive(timeout=2)).type,WSMsgType.CLOSE)

    async def sockets_with_paused_writers(self):
        await self.make_clan()
        await self.join(self.b,'pilot_two')
        await self.join(self.c,'pilot_three')
        relay=self.api.directory.clan_voice
        release=asyncio.Event()
        actual=relay.writer
        async def paused(peer):
            await release.wait()
            await actual(peer)
        patched=patch.object(relay,'writer',side_effect=paused)
        patched.start();self.addCleanup(patched.stop)
        self.addCleanup(release.set)
        sockets=[]
        for headers in (self.a,self.b,self.c):
            socket=await self.client.ws_connect('/v1/clans/voice',headers=headers)
            self.addAsyncCleanup(socket.close);sockets.append(socket)
        return relay,release,sockets

    async def queued(self,relay,count):
        for _ in range(100):
            if relay.relay.stats()['queued']==count:return
            await asyncio.sleep(.01)
        self.fail('Native voice frames did not reach the bounded queue')

    async def test_queued_audio_rechecks_persisted_preferences_before_socket_delivery(self):
        relay,release,(a,b,c)=await self.sockets_with_paused_writers()
        owners=numeric_owners(self.api.directory)
        await a.send_bytes(b'\x01\0\0\0\x01a')
        await self.queued(relay,2)
        response=await self.client.put('/v1/account/voice-blocks',headers={'Authorization':'Bearer one'},json={'blocked':[owners['two']]})
        self.assertEqual(response.status,200,await response.text())
        release.set()
        self.assertEqual((await c.receive(timeout=2)).type,WSMsgType.BINARY)
        with self.assertRaises(asyncio.TimeoutError):await b.receive(timeout=.1)
        self.assertEqual(relay.relay.stats()['queued'],0)

    async def test_replaced_speaker_cannot_deliver_queued_audio_or_remove_the_new_socket(self):
        relay,release,(a,b,c)=await self.sockets_with_paused_writers()
        await a.send_bytes(b'\x01\0\0\0\x01a')
        await self.queued(relay,2)
        newer=await self.client.ws_connect('/v1/clans/voice',headers=self.a)
        self.addAsyncCleanup(newer.close)
        self.assertEqual((await a.receive(timeout=2)).type,WSMsgType.CLOSE)
        release.set()
        with self.assertRaises(asyncio.TimeoutError):await b.receive(timeout=.1)
        await newer.send_bytes(b'\x01\0\0\0\x01b')
        self.assertEqual((await b.receive(timeout=2)).data[-1:],b'b')
        self.assertFalse(newer.closed)
        self.assertEqual(relay.relay.stats()['peers'],3)

    async def test_revocation_during_actual_websocket_upgrade_prevents_attachment(self):
        from aiohttp import web
        await self.make_clan()
        actual=web.WebSocketResponse.prepare
        called=[]
        async def revoked(socket,request):
            result=await actual(socket,request)
            if not called:
                self.api._sessions.clear();called.append(True)
            return result
        with patch.object(web.WebSocketResponse,'prepare',new=revoked):
            socket=await self.client.ws_connect('/v1/clans/voice',headers=self.a)
            self.addAsyncCleanup(socket.close)
            self.assertEqual((await socket.receive(timeout=2)).type,WSMsgType.CLOSE)
        self.assertTrue(called)
        self.assertEqual(self.api.directory.clan_voice.relay.stats()['peers'],0)

    async def test_failed_sql_commit_aborts_an_unpublished_socket_attachment(self):
        from multiplayer.master_server.native_party_directory import run
        await self.make_clan()
        relay=self.api.directory.clan_voice
        auth=self.api._sessions.call('auth',header=self.a['Authorization'],rate=False)
        pending=[]
        def deny(action,operation,column,database,trigger):
            if action==sqlite3.SQLITE_TRANSACTION and operation=='COMMIT':
                pending.append(relay.relay.stats()['pending'])
                return sqlite3.SQLITE_DENY
            return sqlite3.SQLITE_OK
        self.api.db.set_authorizer(deny)
        try:
            with self.assertRaises(sqlite3.DatabaseError):run(relay,'attach',auth['player'],family='voice',account_key=auth['key'])
        finally:self.api.db.set_authorizer(None)
        self.assertIn(1,pending)
        self.assertEqual(relay.relay.stats()['pending'],0)
        self.assertEqual(relay.relay.stats()['peers'],0)

    async def test_expired_commit_plan_cannot_return_an_unpublished_staged_value(self):
        from multiplayer.master_server.native_party_directory import run
        from multiplayer.master_server.protocol import Problem
        await self.make_clan()
        relay=self.api.directory.clan_voice
        now=[time.monotonic()]
        relay.clock=lambda:now[0]
        auth=self.api._sessions.call('auth',header=self.a['Authorization'],rate=False)
        elapsed=[]
        def advance(action,operation,column,database,trigger):
            if action==sqlite3.SQLITE_TRANSACTION and operation=='COMMIT' and relay.relay.stats()['pending']:
                now[0]+=31;elapsed.append(True)
            return sqlite3.SQLITE_OK
        self.api.db.set_authorizer(advance)
        try:
            with self.assertRaises(Problem) as rejected:run(relay,'attach',auth['player'],family='voice',account_key=auth['key'])
            self.assertEqual(rejected.exception.status,409)
        finally:self.api.db.set_authorizer(None)
        self.assertTrue(elapsed)
        self.assertEqual(relay.relay.stats()['peers'],0)
        self.assertEqual(relay.relay.stats()['pending'],0)
