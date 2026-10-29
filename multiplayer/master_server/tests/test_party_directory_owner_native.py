"""Accepted party operations through the authenticated service boundary."""
import unittest
import uuid
from unittest.mock import patch
from multiplayer.master_server.native_party_directory import run
from multiplayer.master_server.native_matchmaking import native, _response
from multiplayer.master_server.matchmaking import Coordinator
from multiplayer.master_server.protocol import Problem
from . import test_parties_http as baseline


class PartyDirectoryOwnerTests(unittest.IsolatedAsyncioTestCase):
    asyncSetUp=baseline.PartiesHttpTests.asyncSetUp
    start=baseline.PartiesHttpTests.start
    login=baseline.PartiesHttpTests.login
    clan_command=baseline.PartiesHttpTests.clan_command
    view=baseline.PartiesHttpTests.view
    command=baseline.PartiesHttpTests.command
    join=baseline.PartiesHttpTests.join
    queue_session=baseline.PartiesHttpTests.queue_session

    async def test_activity_capacity_uses_admitted_local_seats(self):
        await self.command(self.a,'create')
        for headers,name in ((self.b,'pilot_two'),(self.c,'pilot_three'),(self.d,'pilot_four')):
            await self.join(self.a,headers,name)
        version=(await self.view(self.a))['party']['version']
        issued=await self.client.post('/v1/party/activity',headers=self.a,json=dict(version=version))
        self.assertEqual(issued.status,200,await issued.text())
        previous=await issued.json()
        queues=[]
        for owner in ('one','two','three','four'):
            queues.append(await self.queue_session(owner,compatibility='halo2-native-queue-v9',language=0,
                local_players=[dict(screen_name=f'Guest {i}') for i in range(3)]))
        version=(await self.view(self.a))['party']['version']
        response=await self.client.post('/v1/party/activity',headers=self.a,json=dict(version=version))
        self.assertEqual(response.status,409,await response.text())
        self.assertEqual(self.parties.db.execute('SELECT count(*) FROM party_activity_links').fetchone()[0],0)
        self.assertEqual((await self.client.delete('/v1/matchmaking/session',headers=queues[-1])).status,200)
        version=(await self.view(self.a))['party']['version']
        refreshed=await self.client.post('/v1/party/activity',headers=self.a,json=dict(version=version))
        self.assertEqual(refreshed.status,200,await refreshed.text())
        refreshed=await refreshed.json()
        self.assertEqual(refreshed['current_size'],13)
        self.assertNotEqual(refreshed['join_secret'],previous['join_secret'])

    async def test_membership_request_failure_rolls_back_roster_invites_version_and_replay(self):
        await self.command(self.a,'create')
        await self.command(self.a,'invite',username='pilot_two')
        invite=(await self.view(self.b))['invitations'][0]
        db=self.parties.db
        who=db.execute("SELECT player FROM player_profiles WHERE discord_username='pilot_two'").fetchone()[0]
        tables=('parties','party_members','party_invites','party_requests')
        before={table:db.execute('SELECT * FROM '+table+' ORDER BY rowid').fetchall() for table in tables}
        with db:
            db.execute("CREATE TRIGGER reject_party_replay BEFORE INSERT ON party_requests BEGIN SELECT RAISE(ABORT,'declared party request failure'); END")
        body=dict(action='accept',target=invite['id'],version=invite['version'],request_id=str(uuid.uuid4()))
        response=await self.client.post('/v1/party/commands',headers=self.b,json=body)
        self.assertEqual(response.status,500)
        self.assertEqual({table:db.execute('SELECT * FROM '+table+' ORDER BY rowid').fetchall() for table in tables},before)
        with db:db.execute('DROP TRIGGER reject_party_replay')
        first=await self.client.post('/v1/party/commands',headers=self.b,json=body)
        self.assertEqual(first.status,200,await first.text())
        replay=await self.client.post('/v1/party/commands',headers=self.b,json=body)
        self.assertEqual(await first.json(),await replay.json())
        self.assertEqual(db.execute('SELECT count(*) FROM party_members WHERE player=?',(who,)).fetchone()[0],1)

    async def test_directory_reads_do_not_commit_the_callers_open_transaction(self):
        db=self.parties.db
        one,two=[row[0] for row in db.execute('SELECT id FROM players ORDER BY id LIMIT 2')]
        before=db.execute('SELECT * FROM party_blocks ORDER BY rowid').fetchall()
        with self.assertRaisesRegex(RuntimeError,'declared outer rollback'):
            with db:
                db.execute('INSERT INTO party_blocks VALUES(?,?)',(one,two))
                run(self.parties,'party',one)
                raise RuntimeError('declared outer rollback')
        self.assertEqual(db.execute('SELECT * FROM party_blocks ORDER BY rowid').fetchall(),before)

    async def test_python_privacy_alias_cannot_grant_native_closed_party_admission(self):
        await self.command(self.a,'create')
        await self.command(self.a,'invite',username='pilot_two')
        invitation=(await self.view(self.b))['invitations'][0]
        with self.parties.db:self.parties.db.execute('UPDATE party_privacy SET mode=2')
        with patch.object(self.parties,'privacy_state',return_value=(0,1)):
            await self.command(self.b,'accept',403,target=invitation['id'],version=invitation['version'])
        self.assertIsNone((await self.view(self.b))['party'])
        self.assertEqual(len((await self.view(self.b))['invitations']),1)

    async def test_native_directory_cannot_resume_after_its_actual_source_owner_retires(self):
        source=Coordinator(tuple(e.policy for e in self.queue.entries),clock=lambda:10.)
        lib=native()
        handle=None
        try:
            handle=_response(lib.h2_matchmaking_object_open,request=dict(party_directory=dict(
                mode='party',source=source._handle,player='declared-probe-account',wall=100,clock=10.)))
            pending=_response(lib.h2_matchmaking_owner_request,handle,request=dict(op='begin'))
            self.assertEqual(pending['kind'],'sql')
            source.close()
            with self.assertRaises(Problem) as failure:
                _response(lib.h2_matchmaking_owner_request,handle,
                    request=dict(op='reply',frame=pending['frame'],rows=[],changes=[-1]))
            self.assertEqual(failure.exception.status,503)
        finally:
            source.close()
            if handle is not None:lib.h2_matchmaking_owner_drop(handle)
