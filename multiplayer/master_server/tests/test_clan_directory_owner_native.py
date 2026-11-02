"""Native account/clan ownership through authenticated service entry paths."""
import unittest
import hashlib
import uuid
from types import SimpleNamespace
from unittest.mock import patch
from multiplayer.master_server.player_identity import VerifiedOwner
from multiplayer.master_server.native_matchmaking import native,_response
from multiplayer.master_server.protocol import Problem
from . import test_clans_http as baseline


class ClanDirectoryOwnerTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp=baseline.ClansHttpTests.asyncSetUp
    start=baseline.ClansHttpTests.start
    login=baseline.ClansHttpTests.login
    command=baseline.ClansHttpTests.command
    view=baseline.ClansHttpTests.view

    async def test_python_session_bank_cannot_authorize_an_unadmitted_bearer(self):
        token='declared-unadmitted-account-bearer'
        key=hashlib.sha256(token.encode()).hexdigest()
        who=next(iter(self.api.sessions.values())).player
        try:
            self.api.sessions[key]=SimpleNamespace(player=who,expires=self.api.clock()+900,seen=self.api.clock())
        except TypeError:pass
        response=await self.client.get('/v1/clans',headers={'Authorization':'Bearer '+token})
        self.assertEqual(response.status,401,await response.text())

    async def test_account_views_cannot_extend_or_replace_native_admission(self):
        session=next(iter(self.api.sessions.values()))
        with self.assertRaises(AttributeError):session.expires=float('inf')
        with self.assertRaises(AttributeError):self.api.sessions={}
        with self.assertRaises(TypeError):self.api.sessions[session.key]=session
        now=session.expires
        self.api.clock=lambda:now
        self.assertEqual((await self.client.get('/v1/clans',headers=self.a)).status,401)
        self.api.clock=lambda:now-900
        self.assertEqual((await self.client.get('/v1/clans',headers=self.a)).status,401)

    async def test_revoked_account_cannot_resume_a_prepared_clan_directory_transaction(self):
        lib=native()
        key=hashlib.sha256(self.a['Authorization'][7:].encode()).hexdigest()
        player=self.api.sessions[key].player
        handle=_response(lib.h2_matchmaking_object_open,request=dict(party_directory=dict(
            family='clan',mode='get',source=self.api.directory.matchmaking.core._handle,
            player=player,wall=int(self.api.wall()),clock=self.api.directory.matchmaking.clock(),
            account_source=self.api.sessions._handle,account_key=key,account_now=self.api.clock(),features={})))
        try:
            pending=_response(lib.h2_matchmaking_owner_request,handle,request=dict(op='begin',account_now=self.api.clock()))
            self.assertEqual(pending['kind'],'sql')
            self.assertEqual((await self.client.delete('/v1/account/session',headers=self.a)).status,200)
            with self.assertRaises(Problem) as failure:
                _response(lib.h2_matchmaking_owner_request,handle,request=dict(op='reply',frame=pending['frame'],
                    rows=[],changes=[0]*len(pending['statements']),account_now=self.api.clock()))
            self.assertEqual(failure.exception.status,401)
        finally:lib.h2_matchmaking_owner_drop(handle)

    async def test_directory_expiry_observes_time_after_sql_without_another_account_request(self):
        lib=native()
        key=hashlib.sha256(self.a['Authorization'][7:].encode()).hexdigest()
        session=self.api.sessions[key]
        handle=_response(lib.h2_matchmaking_object_open,request=dict(party_directory=dict(
            family='clan',mode='get',source=self.api.directory.matchmaking.core._handle,
            player=session.player,wall=int(self.api.wall()),clock=self.api.directory.matchmaking.clock(),
            account_source=self.api.sessions._handle,account_key=key,account_now=self.api.clock(),features={})))
        try:
            pending=_response(lib.h2_matchmaking_owner_request,handle,request=dict(op='begin',account_now=self.api.clock()))
            # A declared clock crossing at the real reply boundary. No other
            # account operation advances the owner on behalf of this test.
            with self.assertRaises(Problem) as failure:
                _response(lib.h2_matchmaking_owner_request,handle,request=dict(op='reply',frame=pending['frame'],
                    rows=[],changes=[0]*len(pending['statements']),account_now=session.expires))
            self.assertEqual(failure.exception.status,401)
        finally:lib.h2_matchmaking_owner_drop(handle)

    async def test_role_transaction_failure_rolls_back_and_irreversible_promotion_retries_once(self):
        await self.command(self.a,'create',name='Native Clan')
        await self.command(self.a,'invite',username='pilot_two')
        invitation=(await self.view(self.b))['invitations'][0]['id']
        await self.command(self.b,'accept',target=invitation)
        db=self.api.db
        target=db.execute("SELECT player FROM player_profiles WHERE discord_username='pilot_two'").fetchone()[0]
        tables=('clan_members','clan_invites','clan_requests')
        before={table:db.execute('SELECT * FROM '+table+' ORDER BY rowid').fetchall() for table in tables}
        with db:db.execute("CREATE TRIGGER reject_clan_replay BEFORE INSERT ON clan_requests BEGIN SELECT RAISE(ABORT,'declared clan transaction failure'); END")
        body=dict(action='role',request_id=str(uuid.uuid4()),target=target,role=3,confirmed=True)
        response=await self.client.post('/v1/clans/commands',headers=self.a,json=body)
        self.assertEqual(response.status,500)
        self.assertEqual({table:db.execute('SELECT * FROM '+table+' ORDER BY rowid').fetchall() for table in tables},before)
        with db:db.execute('DROP TRIGGER reject_clan_replay')
        first=await self.client.post('/v1/clans/commands',headers=self.a,json=body)
        self.assertEqual(first.status,200,await first.text())
        replay=await self.client.post('/v1/clans/commands',headers=self.a,json=body)
        self.assertEqual(await first.json(),await replay.json())
        self.assertEqual(db.execute('SELECT count(*) FROM clan_members WHERE role=3').fetchone()[0],2)
        await self.command(self.a,'remove',403,target=target)
        await self.command(self.a,'leave')
        self.assertEqual((await self.view(self.b))['clan']['role'],3)

    async def test_python_member_alias_cannot_grant_peon_recruitment(self):
        await self.command(self.a,'create',name='Native Clan')
        await self.command(self.a,'invite',username='pilot_two')
        invitation=(await self.view(self.b))['invitations'][0]['id']
        await self.command(self.b,'accept',target=invitation)
        db=self.api.db
        target=db.execute("SELECT player FROM player_profiles WHERE discord_username='pilot_two'").fetchone()[0]
        await self.command(self.a,'role',target=target,role=0,confirmed=True)
        stored=self.api.member(target)
        with patch.object(self.api,'member',return_value=(stored[0],3,stored[2])):
            await self.command(self.b,'invite',403,username='pilot_three')
        self.assertEqual((await self.view(self.c))['invitations'],[])

    async def test_account_capacity_rejection_precedes_persistent_identity_and_history_io(self):
        directory=self.api.directory
        async def verifier(session,token,application):
            if token.startswith('bulk'):
                return VerifiedOwner('discord','declared-capacity-subject-'+token,'pilot_'+token)
            return await self.verifier(session,token,application)
        # This is a declared capacity fixture. Disable outer IP/provider rate
        # limits while exercising1000 actual verified account admissions;
        # native account/session capacity and authorization stay enabled.
        with patch.object(directory,'verify_owner',new=verifier),patch.object(directory,'rate',return_value=None):
            first=None
            for i in range(996):
                headers=await self.login('bulk'+str(i))
                if i==0:first=headers
            self.assertEqual(len(self.api.sessions),1000)
            tables=('players','player_profiles','player_identities','history_players','player_friend_tags','friend_cloud_outbox')
            before={table:self.api.db.execute('SELECT count(*) FROM '+table).fetchone()[0] for table in tables}
            response=await self.client.post('/v1/account/session',headers={'Authorization':'Bearer bulk996'},json={'screen_name':'Denied capacity fixture'})
            self.assertEqual(response.status,503,await response.text())
            self.assertEqual({table:self.api.db.execute('SELECT count(*) FROM '+table).fetchone()[0] for table in tables},before)
            replacement=await self.login('bulk0')
            self.assertEqual(len(self.api.sessions),1000)
            self.assertEqual((await self.client.get('/v1/clans',headers=first)).status,401)
            self.assertEqual((await self.client.get('/v1/clans',headers=replacement)).status,200)
