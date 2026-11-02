import sqlite3
import unittest
import unicodedata
import uuid
from unittest.mock import patch
from . import test_friends_http as friends
from . import test_friend_cloud as cloud


class FriendUnicodeOwnerTests(unittest.TestCase):
    @unittest.skipUnless(unicodedata.unidata_version=='16.0.0','Differential requires the prior Unicode16 Python reference')
    def test_native_unicode16_fold_matches_every_valid_scalar(self):
        from multiplayer.master_server.friend_tags import fold
        pieces=[];size=0;count=0
        def check():
            text=''.join(pieces)
            self.assertEqual(fold(text),unicodedata.normalize('NFC',text).casefold())
        for code in range(0x110000):
            if 0xd800<=code<=0xdfff:continue
            char=chr(code)+'\0'
            width=len(char.encode())
            if size+width>1000:
                check();pieces=[];size=0
            pieces.append(char);size+=width;count+=1
        if pieces:check()
        self.assertEqual(count,1112064)


class FriendOwnerTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = friends.FriendsHttpTests.asyncSetUp
    start = friends.FriendsHttpTests.start
    login = friends.FriendsHttpTests.login
    view = friends.FriendsHttpTests.view
    command = friends.FriendsHttpTests.command
    find = friends.FriendsHttpTests.find

    async def test_python_block_cache_alias_cannot_bypass_a_persisted_block(self):
        a = await self.find(self.b, self.a)
        b = await self.find(self.a, self.b)
        await self.command(self.b, 'block', target=a['id'])
        self.api.directory.friends.blocks = set()
        await self.command(self.a, 'request', 404, target=b['id'])
        self.assertEqual(self.api.db.execute('SELECT count(*) FROM player_friendships').fetchone()[0], 0)

    async def test_friend_command_preserves_an_outer_transaction_and_rollback(self):
        b = await self.find(self.a, self.b)
        db = self.api.db
        db.execute('CREATE TEMP TABLE friend_outer_marker(value INTEGER)')
        db.execute('BEGIN')
        db.execute('INSERT INTO friend_outer_marker VALUES(117)')
        try:
            await self.command(self.a, 'block', target=b['id'])
            self.assertTrue(db.in_transaction)
            self.assertEqual(db.execute('SELECT value FROM friend_outer_marker').fetchone()[0], 117)
        finally:
            db.rollback()
        self.assertEqual(db.execute('SELECT count(*) FROM friend_outer_marker').fetchone()[0], 0)
        self.assertEqual(db.execute('SELECT count(*) FROM player_friend_blocks').fetchone()[0], 0)

    async def test_revocation_at_a_provider_boundary_rolls_back_friend_write_and_replay(self):
        from multiplayer.master_server import player_presence
        b = await self.find(self.a, self.b)
        directory = self.api.directory
        directory.friend_cloud.enabled = True
        who = directory.friends.target((await self.find(self.b, self.a))['id'])
        actual = player_presence.observed
        called = []
        def revoked_presence(source):
            if not called:
                directory.clans._sessions.revoke(who)
                called.append(True)
            return actual(source)
        body = dict(action='request', request_id=str(uuid.uuid4()), target=b['id'])
        with patch.object(player_presence, 'observed', side_effect=revoked_presence):
            response = await self.client.post('/v1/friends/commands', headers=self.a, json=body)
        self.assertTrue(called)
        self.assertEqual(response.status, 401, await response.text())
        self.assertEqual(self.api.db.execute('SELECT count(*) FROM player_friendships').fetchone()[0], 0)
        self.assertEqual(self.api.db.execute('SELECT count(*) FROM player_friend_commands').fetchone()[0], 0)
        self.assertEqual(self.api.db.execute('SELECT count(*) FROM friend_cloud_outbox').fetchone()[0], 0)

    async def test_another_sql_connection_invalidates_a_cached_allow_in_both_directions(self):
        api = self.api.directory.friends
        a = api.target((await self.find(self.b,self.a))['id'])
        b = api.target((await self.find(self.a,self.b))['id'])
        self.assertFalse(api.blocked(a,b))
        with sqlite3.connect(self.api.directory.settings.state/'registry.sqlite3') as writer:
            writer.execute('INSERT INTO player_friend_blocks VALUES(?,?)',(b,a))
        self.assertTrue(api.blocked(a,b))
        self.assertTrue(api.blocked(b,a))
        with sqlite3.connect(self.api.directory.settings.state/'registry.sqlite3') as writer:
            writer.execute('DELETE FROM player_friend_blocks WHERE player=? AND target=?',(b,a))
        self.assertFalse(api.blocked(a,b))

    async def test_failed_sql_commit_aborts_a_staged_friend_cache_entry(self):
        api = self.api.directory.friends
        a = api.target((await self.find(self.b,self.a))['id'])
        b = api.target((await self.find(self.a,self.b))['id'])
        commits = []
        def deny(action, operation, column, database, trigger):
            if action == sqlite3.SQLITE_TRANSACTION and operation == 'COMMIT':
                commits.append(api.cache.stats()['pending'])
                return sqlite3.SQLITE_DENY
            return sqlite3.SQLITE_OK
        self.api.db.set_authorizer(deny)
        try:
            with self.assertRaises(sqlite3.DatabaseError):api.blocked(a,b)
        finally:self.api.db.set_authorizer(None)
        self.assertIn(1,commits)
        self.assertEqual(api.cache.stats()['pending'],0)
        self.assertEqual(api.cache.stats()['cached'],0)
        self.assertFalse(api.blocked(a,b))
        self.assertEqual(api.cache.stats()['cached'],1)


class FriendCloudOwnerTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp = cloud.FriendCloudTests.asyncSetUp
    start = cloud.FriendCloudTests.start
    login = cloud.FriendCloudTests.login
    view = cloud.FriendCloudTests.view
    command = cloud.FriendCloudTests.command
    find = cloud.FriendCloudTests.find
    bind = cloud.FriendCloudTests.bind
    row = cloud.FriendCloudTests.row

    async def test_damaged_persisted_digest_cannot_become_an_upload_claim(self):
        from multiplayer.master_server.protocol import Problem
        self.bind()
        with self.api.db:
            self.api.db.execute("UPDATE friend_cloud_outbox SET digest=? WHERE player=?",('0'*64,self.who))
        with self.assertRaises(Problem) as rejected:
            await self.cloud.flush()
        self.assertEqual(rejected.exception.status,503)
        self.assertEqual(self.uploads,[])
        self.assertEqual(self.row()['uploaded_revision'],0)

    async def test_cloud_upload_has_no_open_sql_transaction(self):
        self.bind()
        observed = []
        async def upload(*_):observed.append(self.api.db.in_transaction)
        self.cloud.upload = upload
        self.assertTrue(await self.cloud.flush())
        self.assertEqual(observed, [False])

    async def test_cloud_cannot_publish_an_uncommitted_outer_transaction(self):
        self.bind()
        db = self.api.db
        before = self.row()['revision']
        db.execute('BEGIN')
        try:
            db.execute("UPDATE player_profiles SET screen_name='Uncommitted' WHERE player=?", (self.who,))
            self.cloud.enqueue(self.who)
            self.assertEqual(self.row()['revision'], before+1)
            self.assertFalse(await self.cloud.flush())
            self.assertEqual(self.uploads, [])
            self.assertTrue(db.in_transaction)
        finally:
            db.rollback()
        self.assertEqual(self.row()['revision'], before)
