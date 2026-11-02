"""Canonical identity transactions through real SQLite and account HTTP.

Provider responses are deterministic fixtures; no live account is linked.
"""
import hashlib
import sqlite3
import unittest
import uuid
from unittest.mock import patch

from ..player_identity import initialize, player, allocate, VerifiedOwner
from . import test_steam_login as steam


class IdentityTransactionTests(unittest.TestCase):
    def test_schema_and_low_level_allocation_preserve_caller_rollback(self):
        db=sqlite3.connect(':memory:')
        try:
            db.execute('CREATE TABLE identity_marker(value TEXT)');db.commit()
            db.execute('BEGIN');db.execute("INSERT INTO identity_marker VALUES('outer')")
            initialize(db)
            self.assertTrue(db.in_transaction)
            allocate(db,('steam','fixture'))
            self.assertEqual(db.execute('SELECT count(*) FROM players').fetchone()[0],1)
            db.rollback()
            self.assertEqual(db.execute('SELECT count(*) FROM identity_marker').fetchone()[0],0)
            self.assertIsNone(db.execute("SELECT 1 FROM sqlite_master WHERE name='players'").fetchone())
        finally:db.close()

    def test_failed_resolution_rolls_back_its_writes_and_keeps_the_callers_marker(self):
        db=sqlite3.connect(':memory:')
        try:
            initialize(db)
            db.executescript("CREATE TABLE identity_marker(value TEXT);"
                "CREATE TRIGGER fail_identity_tag BEFORE INSERT ON player_friend_tags BEGIN SELECT RAISE(ABORT,'declared tag failure'); END;")
            db.execute('BEGIN');db.execute("INSERT INTO identity_marker VALUES('outer')")
            with self.assertRaises(sqlite3.IntegrityError):player(db,('steam','fixture'),'Pilot','Persona')
            self.assertTrue(db.in_transaction)
            self.assertEqual(db.execute('SELECT count(*) FROM identity_marker').fetchone()[0],1)
            self.assertEqual(db.execute('SELECT count(*) FROM players').fetchone()[0],0)
            db.rollback()
        finally:db.close()

    def test_resolution_does_not_commit_a_callers_outer_transaction(self):
        db=sqlite3.connect(':memory:')
        try:
            initialize(db)
            db.execute('CREATE TABLE identity_marker(value TEXT)');db.commit()
            db.execute('BEGIN')
            db.execute("INSERT INTO identity_marker VALUES('outer')")
            try:
                player(db,('steam','fixture-primary'),'Pilot','Persona',now=1700000000)
                self.assertTrue(db.in_transaction,'Canonical resolution committed the caller transaction')
            finally:db.rollback()
            self.assertEqual(db.execute('SELECT count(*) FROM identity_marker').fetchone()[0],0)
            self.assertEqual(db.execute('SELECT count(*) FROM players').fetchone()[0],0)
        finally:db.close()


class IdentityLinkOwnerTests(unittest.IsolatedAsyncioTestCase):
    start=steam.SteamLoginTests.start
    signature=steam.SteamLoginTests.signature
    assertion=steam.SteamLoginTests.assertion
    complete=steam.SteamLoginTests.complete

    async def asyncSetUp(self):
        await steam.SteamLoginTests.asyncSetUp(self)
        response=await self.client.post('/v1/account/session',headers={'Authorization':'Bearer legacy'},json={'screen_name':'Retained Pilot'})
        self.assertEqual(response.status,201,await response.text())
        self.discord=self.steam.db.execute("SELECT player FROM player_identities WHERE provider='discord'").fetchone()[0]
        _,self.identity=await self.complete()
        self.header={'Authorization':'Bearer '+self.identity['token']}
        response=await self.client.post('/v1/account/session',headers=self.header,json={'screen_name':'New Pilot'})
        self.assertEqual(response.status,201,await response.text())
        self.account={'Authorization':'Bearer '+(await response.json())['token']}
        self.who=self.steam.db.execute("SELECT player FROM player_identities WHERE provider='steam'").fetchone()[0]

    async def test_expired_primary_after_discord_provider_await_cannot_link(self):
        original=self.directory.verify_owner
        async def expired(*args):
            result=await original(*args)
            self.now+=28801
            return result
        with patch.object(self.directory,'verify_owner',side_effect=expired):
            response=await self.client.post('/v1/account/link-discord',headers=self.header,json={'discord_token':'legacy'})
        self.assertEqual(response.status,401,await response.text())
        self.assertEqual(self.steam.db.execute("SELECT player FROM player_identities WHERE provider='steam'").fetchone()[0],self.who)

    async def test_revoked_primary_after_discord_provider_await_cannot_link(self):
        original=self.directory.verify_owner
        async def revoked(*args):
            result=await original(*args)
            with self.steam.db:
                self.steam.db.execute('DELETE FROM steam_login_sessions WHERE token_hash=?',(hashlib.sha256(self.identity['token'].encode()).hexdigest(),))
            return result
        with patch.object(self.directory,'verify_owner',side_effect=revoked):
            response=await self.client.post('/v1/account/link-discord',headers=self.header,json={'discord_token':'legacy'})
        self.assertEqual(response.status,401,await response.text())
        self.assertEqual(self.steam.db.execute("SELECT player FROM player_identities WHERE provider='steam'").fetchone()[0],self.who)

    async def test_outer_link_transaction_rejects_before_discord_verification(self):
        db=self.steam.db
        db.execute('CREATE TABLE identity_marker(value TEXT)');db.commit()
        db.execute('BEGIN');db.execute("INSERT INTO identity_marker VALUES('outer')")
        try:
            with patch.object(self.directory,'verify_owner') as provider:
                response=await self.client.post('/v1/account/link-discord',headers=self.header,json={'discord_token':'legacy'})
            self.assertEqual(response.status,409,await response.text())
            provider.assert_not_called()
            self.assertTrue(db.in_transaction)
        finally:db.rollback()
        self.assertEqual(db.execute('SELECT count(*) FROM identity_marker').fetchone()[0],0)
        self.assertEqual(db.execute("SELECT player FROM player_identities WHERE provider='steam'").fetchone()[0],self.who)

    async def test_failed_commit_preserves_the_mapping_and_actual_native_account(self):
        db=self.steam.db
        with db:
            db.executescript("CREATE TABLE identity_commit_failure(player TEXT REFERENCES players(id) DEFERRABLE INITIALLY DEFERRED);"
                "CREATE TRIGGER fail_identity_commit AFTER DELETE ON player_profiles BEGIN INSERT INTO identity_commit_failure VALUES('declared-missing-player'); END;")
        checks=[]
        def observe(action,a,b,c,d):
            if action==sqlite3.SQLITE_TRANSACTION and a=='COMMIT':
                checks.append(any(s.player==self.who for s in self.directory.clans.sessions.values()))
            return sqlite3.SQLITE_OK
        db.set_authorizer(observe)
        try:response=await self.client.post('/v1/account/link-discord',headers=self.header,json={'discord_token':'legacy'})
        finally:db.set_authorizer(None)
        self.assertEqual(response.status,500,await response.text())
        self.assertTrue(checks and all(checks))
        self.assertEqual(db.execute("SELECT player FROM player_identities WHERE provider='steam'").fetchone()[0],self.who)
        self.assertEqual(db.execute('SELECT count(*) FROM identity_commit_failure').fetchone()[0],0)
        self.assertEqual((await self.client.get('/v1/account/profile',headers=self.account)).status,200)
        with db:db.execute('DROP TRIGGER fail_identity_commit')
        response=await self.client.post('/v1/account/link-discord',headers=self.header,json={'discord_token':'legacy'})
        self.assertEqual(response.status,200,await response.text())
        self.assertEqual(await response.json(),{'state':'linked'})
        self.assertEqual(db.execute("SELECT player FROM player_identities WHERE provider='steam'").fetchone()[0],self.discord)
        self.assertEqual((await self.client.get('/v1/account/profile',headers=self.account)).status,401)

    async def test_current_signed_local_seat_blocks_adoption_even_with_an_empty_python_projection(self):
        response=await self.client.post('/v1/matchmaking/session',headers={'Authorization':'Bearer legacy'},json={
            'screen_name':'Retained Pilot','maps':['lockout'],'compatibility':'halo2-native-queue-v12','language':0,
            'local_players':[{'screen_name':'Signed Local','discord_token':self.identity['token']}]})
        self.assertEqual(response.status,201,await response.text())
        with patch.object(self.directory.matchmaking.sessions,'values',return_value=[]):
            response=await self.client.post('/v1/account/link-discord',headers=self.header,json={'discord_token':'legacy'})
        self.assertEqual(response.status,409,await response.text())
        self.assertEqual(self.steam.db.execute("SELECT player FROM player_identities WHERE provider='steam'").fetchone()[0],self.who)

    async def test_quoted_persistent_foreign_key_blocks_adoption(self):
        db=self.steam.db
        with db:
            db.execute('CREATE TABLE "identity ""quoted table"("own""er" TEXT REFERENCES players(id))')
            db.execute('INSERT INTO "identity ""quoted table" VALUES(?)',(self.who,))
        response=await self.client.post('/v1/account/link-discord',headers=self.header,json={'discord_token':'legacy'})
        self.assertEqual(response.status,409,await response.text())
        self.assertEqual(db.execute('SELECT * FROM "identity ""quoted table"').fetchone()[0],self.who)

    async def test_foreign_keys_after_the_first_schema_page_are_checked(self):
        db=self.steam.db
        with db:
            for i in range(130):db.execute(f'CREATE TABLE a_identity_fixture_{i:03d}(value TEXT)')
            db.execute('CREATE TABLE "zz_identity\nreference"("owner\ncolumn" TEXT REFERENCES players(id))')
            db.execute('INSERT INTO "zz_identity\nreference" VALUES(?)',(self.who,))
        response=await self.client.post('/v1/account/link-discord',headers=self.header,json={'discord_token':'legacy'})
        self.assertEqual(response.status,409,await response.text())
        self.assertEqual(db.execute('SELECT * FROM "zz_identity\nreference"').fetchone()[0],self.who)

    async def test_new_link_registration_uses_the_persisted_clock_floor(self):
        _,identity=await self.complete('76561197960287931')
        db=self.steam.db
        floor=db.execute('SELECT value FROM clock_floor WHERE id=1').fetchone()[0]+100
        with db:db.execute('UPDATE clock_floor SET value=? WHERE id=1',(floor,))
        async def social(*args):return VerifiedOwner('discord','fresh-fixture-subject','fresh_pilot')
        with patch.object(self.directory,'verify_owner',side_effect=social),patch.object(self.directory.store,'clock',return_value=floor-1000):
            response=await self.client.post('/v1/account/link-discord',headers={'Authorization':'Bearer '+identity['token']},json={'discord_token':'fresh'})
        self.assertEqual(response.status,200,await response.text())
        joined=db.execute("SELECT joined_at FROM player_profiles p JOIN player_identities i ON i.player=p.player WHERE i.provider='steam' AND i.subject=?",(identity['identity']['id'],)).fetchone()[0]
        self.assertEqual(joined,floor)
        self.assertEqual(db.execute('SELECT value FROM clock_floor WHERE id=1').fetchone()[0],floor)

    async def test_couch_identity_allocation_at_capacity_is_one_atomic_operation(self):
        db=self.steam.db
        with db:
            count=db.execute('SELECT count(*) FROM players').fetchone()[0]
            db.executemany('INSERT INTO players VALUES(?,?,?)',[(str(uuid.uuid4()),'fixture',f'capacity-{i}') for i in range(999-count)])
        tables=('players','player_profiles','player_identities','player_friend_tags')
        before=[db.execute('SELECT count(*) FROM '+table).fetchone()[0] for table in tables]
        async def social(session,token,application):return VerifiedOwner('discord','fresh-'+token,'fresh_'+token)
        with patch.object(self.directory,'verify_owner',side_effect=social):
            response=await self.client.post('/v1/matchmaking/session',headers={'Authorization':'Bearer primary'},json={
                'screen_name':'Primary','maps':['lockout'],'compatibility':'halo2-native-queue-v12','language':0,
                'local_players':[{'screen_name':'Signed Local','identity_token':'local'}]})
        self.assertEqual(response.status,503,await response.text())
        self.assertEqual([db.execute('SELECT count(*) FROM '+table).fetchone()[0] for table in tables],before,
                         'Rejected couch registration retained a partly allocated canonical roster')
        self.assertEqual(len(self.directory.matchmaking.sessions),0)

    async def test_new_primary_and_signed_local_get_distinct_native_canonical_identities(self):
        async def social(session,token,application):return VerifiedOwner('discord','fresh-'+token,'fresh_'+token)
        with patch.object(self.directory,'verify_owner',side_effect=social):
            response=await self.client.post('/v1/matchmaking/session',headers={'Authorization':'Bearer primary'},json={
                'screen_name':'Primary','maps':['lockout'],'compatibility':'halo2-native-queue-v12','language':0,
                'local_players':[{'screen_name':'Signed Local','identity_token':'local'},{'screen_name':'Guest'}]})
        self.assertEqual(response.status,201,await response.text())
        rows=list(self.steam.db.execute("SELECT player FROM player_identities WHERE provider='discord' AND subject IN ('fresh-primary','fresh-local')"))
        self.assertEqual(len({row[0] for row in rows}),2)
        for row in rows:self.assertTrue(self.directory.matchmaking.core._call('session_identity_in_use',player=row[0]))
        self.assertEqual(len(self.directory.matchmaking.sessions),1)
        self.assertEqual(self.steam.db.execute("SELECT count(*) FROM players WHERE subject LIKE 'fresh-%'").fetchone()[0],2)
