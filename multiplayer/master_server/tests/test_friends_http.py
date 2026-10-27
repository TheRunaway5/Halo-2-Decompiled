import json
import sqlite3
import unittest
import uuid
from contextlib import closing
from unittest.mock import patch

from . import test_clans_http as clans
from .. import friend_tags
from ..player_identity import initialize, player


class FriendTagTests(unittest.TestCase):
    def test_lazy_migration_collision_retry_rename_and_unicode(self):
        with closing(sqlite3.connect(":memory:")) as db:
            initialize(db)
            db.execute("INSERT INTO players VALUES('old','steam','verified-old')")
            db.execute("INSERT INTO player_profiles(player,screen_name) VALUES('old','Old')")
            initialize(db)
            self.assertIsNone(friend_tags.tag(db,"old"))
            with patch.object(friend_tags.secrets,"randbelow",side_effect=[1,1,99999999]):
                a=player(db,("steam","verified-a"),"Café","Persona")
                b=player(db,("steam","verified-b"),"Café","Persona")
            self.assertEqual(friend_tags.tag(db,a),"Café#00000001")
            self.assertEqual(friend_tags.tag(db,b),"Café#99999999")
            self.assertEqual(friend_tags.resolve(db,"CAFE\u0301#00000001"),a)
            self.assertEqual(player(db,("steam","verified-a"),"Renamed","New Persona"),a)
            self.assertEqual(friend_tags.tag(db,a),"Renamed#00000001")
            with self.assertRaisesRegex(Exception,"Player not found"):
                friend_tags.resolve(db,"Café#00000001")
            player(db,("steam","verified-old"),"Old","Persona")
            self.assertRegex(friend_tags.tag(db,"old"),r"^Old#[0-9]{8}$")
            self.assertIsNone(db.execute("SELECT joined_at FROM player_profiles WHERE player='old'").fetchone()[0])


class FriendsHttpTests(unittest.IsolatedAsyncioTestCase):
    start=clans.ClansHttpTests.start
    login=clans.ClansHttpTests.login
    asyncSetUp=clans.ClansHttpTests.asyncSetUp

    async def view(self,headers):
        r=await self.client.get('/v1/friends',headers=headers)
        self.assertEqual(r.status,200,await r.text())
        return await r.json()

    async def command(self,headers,action,status=200,**kw):
        body=dict(request_id=str(uuid.uuid4()),action=action)|kw
        r=await self.client.post('/v1/friends/commands',headers=headers,json=body)
        value=await r.json()
        self.assertEqual(r.status,status,value)
        return value

    async def find(self,headers,target):
        tag=(await self.view(target))['friend_tag']
        return (await self.command(headers,'search',tag=tag))['search_result']

    async def test_search_consent_privacy_and_rename_across_connections(self):
        b=await self.find(self.a,self.b)
        self.assertEqual(b['name'],'Chosen Pilot')
        self.assertRegex(b['friend_tag'],r'^Chosen Pilot#[0-9]{8}$')
        self.assertEqual(b['presence'],'offline','Searching cannot reveal a nonfriend presence')
        a=await self.find(self.b,self.a)
        sent=await self.command(self.a,'request',target=b['id'])
        self.assertEqual(sent['friends'][0]['state'],'outgoing')
        await self.command(self.a,'accept',409,target=b['id'])
        self.assertEqual((await self.view(self.b))['friends'][0]['state'],'incoming')
        accepted=await self.command(self.b,'accept',target=a['id'])
        self.assertEqual(accepted['friends'][0]['state'],'friend')
        renamed=await self.login('two','New Name')
        current=(await self.view(self.a))['friends'][0]
        self.assertEqual(current['id'],b['id'])
        self.assertEqual(current['name'],'New Name')
        self.assertEqual(current['friend_tag'].rpartition('#')[2],b['friend_tag'].rpartition('#')[2])
        await self.command(self.a,'search',404,tag=b['friend_tag'])
        self.assertEqual((await self.find(self.a,renamed))['id'],b['id'])
        await self.client.close()
        await self.start()
        self.a=await self.login('one')
        self.assertEqual((await self.view(self.a))['friends'][0]['id'],b['id'])
        private=[r[0] for r in self.api.db.execute('SELECT id FROM players')]
        for value in (*private,'private-discord','steamid','subject','provider'):
            self.assertNotIn(value,json.dumps(current))

    async def test_cancel_decline_remove_block_unblock_and_no_auto_accept(self):
        a=await self.find(self.b,self.a);b=await self.find(self.a,self.b)
        await self.command(self.a,'request',target=b['id'])
        await self.command(self.b,'request',409,target=a['id'])
        await self.command(self.b,'cancel',409,target=a['id'])
        await self.command(self.a,'cancel',target=b['id'])
        await self.command(self.a,'request',target=b['id'])
        await self.command(self.b,'decline',target=a['id'])
        await self.command(self.a,'request',target=b['id'])
        await self.command(self.b,'accept',target=a['id'])
        await self.command(self.a,'remove',target=b['id'])
        await self.command(self.a,'request',target=b['id'])
        await self.command(self.b,'block',target=a['id'])
        self.assertEqual((await self.view(self.a))['friends'],[])
        self.assertEqual((await self.view(self.b))['friends'][0]['state'],'blocked')
        await self.command(self.a,'request',404,target=b['id'])
        await self.command(self.a,'search',404,tag=b['friend_tag'])
        friends=self.api.directory.friends
        self.assertFalse(self.api.directory.voice_blocks.allowed(friends.target(a['id']),friends.target(b['id'])))
        await self.client.close()
        await self.start()
        self.a=await self.login('one');self.b=await self.login('two')
        friends=self.api.directory.friends
        self.assertTrue(friends.blocked(friends.target(a['id']),friends.target(b['id'])))
        await self.command(self.b,'unblock',target=a['id'])
        self.assertEqual((await self.view(self.b))['friends'],[])
        await self.command(self.a,'request',target=b['id'])

    async def test_commands_are_authorized_idempotent_and_reject_forged_fields(self):
        self.assertEqual((await self.client.get('/v1/friends')).status,401)
        r=await self.client.post('/v1/friends/commands',data=b'not-json',headers={'Content-Type':'application/octet-stream'})
        self.assertEqual(r.status,401,'Unauthenticated requests must be rejected before body parsing')
        b=await self.find(self.a,self.b)
        body=dict(request_id=str(uuid.uuid4()),action='request',target=b['id'])
        for _ in range(2):
            self.assertEqual((await self.client.post('/v1/friends/commands',headers=self.a,json=body)).status,200)
        self.assertEqual(len((await self.view(self.b))['friends']),1)
        self.assertEqual((await self.client.post('/v1/friends/commands',headers=self.a,json=body|dict(action='remove'))).status,409)
        self.assertEqual((await self.client.post('/v1/friends/commands',headers=self.a,json=body|dict(player='forged'))).status,400)
        for tag in ('Name#1234','Name#１２３４５６７８','Name#123456789','\nName#12345678'):
            await self.command(self.a,'search',400,tag=tag)

    async def test_friend_party_invitation_and_block_revocation(self):
        a=await self.find(self.b,self.a);b=await self.find(self.a,self.b)
        r=await self.client.post('/v1/party/commands',headers=self.a,json=dict(action='create',request_id=str(uuid.uuid4())))
        self.assertEqual(r.status,200)
        async def invite(status):
            r=await self.client.post('/v1/party/commands',headers=self.a,json=dict(
                action='invite_friend',request_id=str(uuid.uuid4()),target=b['id'],version=1))
            self.assertEqual(r.status,status,await r.text())
        await invite(403)
        await self.command(self.a,'request',target=b['id'])
        await self.command(self.b,'accept',target=a['id'])
        await invite(200)
        self.assertEqual(len(self.api.db.execute('SELECT id FROM party_invites').fetchall()),1)
        await self.command(self.b,'block',target=a['id'])
        self.assertEqual(self.api.db.execute('SELECT id FROM party_invites').fetchall(),[])
        await invite(403)

    async def test_full_roster_uses_bounded_queries_and_capacity_is_enforced(self):
        db=self.api.db
        who=db.execute("SELECT player FROM player_identities WHERE subject='private-discord-subject-one'").fetchone()[0]
        with db:
            for i in range(250):
                target=player(db,('steam',f'verified-roster-{i}'),'W'*64,'Persona')
                self.api.directory.matchmaking.results.history.identity(target)
                db.execute('INSERT INTO player_friendships VALUES(?,?,?,1)',(*sorted((who,target)),who))
        queries=[];db.set_trace_callback(queries.append)
        try:
            view=self.api.directory.friends.view(who)
        finally:
            db.set_trace_callback(None)
        self.assertEqual(len(view['friends']),250)
        self.assertLess(len(json.dumps(view).encode()),256*1024)
        self.assertLessEqual(len(queries),3,'A roster must not issue queries for every friend')
        b=await self.find(self.a,self.b)
        await self.command(self.a,'request',409,target=b['id'])

    async def test_pending_request_spam_is_bounded_separately_from_accepted_friends(self):
        db=self.api.db;api=self.api.directory.friends
        a=await self.find(self.b,self.a);b=await self.find(self.a,self.b)
        who=api.target(a['id']);peer=api.target(b['id'])
        # A full incoming inbox cannot exhaust the total roster or prevent
        # accepting and blocking requests already received.
        with db:
            for i in range(32):
                attacker=player(db,('steam',f'spam-{i}'),'Spam','Persona')
                self.api.directory.matchmaking.results.history.identity(attacker)
                db.execute('INSERT INTO player_friendships VALUES(?,?,?,0)',(*sorted((attacker,who)),attacker))
        await self.command(self.b,'request',429,target=a['id'])
        self.assertLess(len((await self.view(self.a))['friends']),250)
        existing=(await self.view(self.a))['friends'][0]
        await self.command(self.a,'accept',target=existing['id'])
        await self.command(self.b,'request',target=a['id'])
        await self.command(self.a,'block',target=b['id'])
        self.assertTrue(api.blocked(who,peer))
        # A sender cannot keep an arbitrarily large pending queue either.
        with db:
            db.execute('DELETE FROM player_friendships')
            for i in range(32):
                target=player(db,('steam',f'pending-{i}'),'Target','Persona')
                self.api.directory.matchmaking.results.history.identity(target)
                db.execute('INSERT INTO player_friendships VALUES(?,?,?,0)',(*sorted((peer,target)),peer))
        c=await self.find(self.b,self.c)
        await self.command(self.b,'request',429,target=c['id'])
