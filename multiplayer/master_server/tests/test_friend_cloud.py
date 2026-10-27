import asyncio
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

from . import test_friends_http as friends
from ..friend_cloud import private_config


def config_file(root):
    path=Path(root)/'private.env'
    values=dict(H2_EOS_PRODUCT_ID='a'*32,H2_EOS_SANDBOX_ID='b'*32,H2_EOS_DEPLOYMENT_ID='c'*32,
        H2_EOS_FRIEND_CLIENT_ID='dedicated-client',H2_EOS_FRIEND_CLIENT_SECRET='private-storage-secret',
        H2_EOS_FRIEND_CACHE_DIR=str(Path(root)/'cache'),H2_EOS_FRIEND_ENCRYPTION_KEY='d'*64)
    path.write_text('\n'.join(k+'='+v for k,v in values.items())+'\n');path.chmod(0o600)
    return path


class FriendConfigTests(unittest.TestCase):
    def test_private_config_rejects_public_symlink_large_duplicate_or_unrelated_secrets(self):
        with tempfile.TemporaryDirectory() as root:
            path=config_file(root);valid=path.read_text()
            self.assertEqual(private_config(str(path))['H2_EOS_FRIEND_CLIENT_ID'],'dedicated-client')
            if os.name=='posix':
                path.chmod(0o644)
                with self.assertRaisesRegex(Exception,'owner-only'):private_config(str(path))
                path.chmod(0o600)
                link=Path(root)/'link.env';link.symlink_to(path)
                with self.assertRaises(OSError):private_config(str(link))
            for bad in (valid*2,valid+'H2_EOS_SERVER_CLIENT_SECRET=unrelated\n','X'*16385):
                path.write_text(bad)
                with self.assertRaises(Exception):private_config(str(path))
            with self.assertRaisesRegex(Exception,'absolute'):private_config('private.env')


class FriendCloudTests(unittest.IsolatedAsyncioTestCase):
    start=friends.FriendsHttpTests.start
    login=friends.FriendsHttpTests.login
    view=friends.FriendsHttpTests.view
    command=friends.FriendsHttpTests.command
    find=friends.FriendsHttpTests.find
    # Exercise the real HTTP consent and transaction paths with an owned,
    # offline cloud transport. Never operate on real player cloud files here.
    async def asyncSetUp(self):
        scoped=patch.dict(os.environ);scoped.start();self.addCleanup(scoped.stop)
        os.environ.pop('H2_EOS_FRIEND_CACHE_DIR',None)
        with patch.dict(os.environ,{'H2_EOS_FRIEND_STORAGE':'0'}):
            await friends.FriendsHttpTests.asyncSetUp(self)
        self.cloud=self.api.directory.friend_cloud
        self.cloud.enabled=True
        self.who=self.api.db.execute("SELECT player FROM player_identities WHERE subject='private-discord-subject-one'").fetchone()[0]
        self.peer=self.api.db.execute("SELECT player FROM player_identities WHERE subject='private-discord-subject-two'").fetchone()[0]
        self.cloud.directory.eos_identity.token=lambda who,name:'private-delegation-for-'+who
        self.uploads=[]
        async def upload(puid,token,payload):
            self.assertEqual(token,'private-delegation-for-'+self.who)
            self.uploads.append((puid,json.loads(payload)))
        self.cloud.upload=upload

    def row(self,who=None):
        return self.api.db.execute('SELECT * FROM friend_cloud_outbox WHERE player=?',(who or self.who,)).fetchone()

    def bind(self):
        self.api.directory.eos_identity.bind_player(self.who,'a'*32)

    async def test_both_participants_queued_and_offline_peer_waits_for_binding(self):
        target=await self.find(self.a,self.b)
        await self.command(self.a,'request',target=target['id'])
        self.assertEqual(json.loads(self.row()['payload'])['relationships'],[{'player':target['id'],'state':'outgoing'}])
        self.assertEqual(json.loads(self.row(self.peer)['payload'])['relationships'][0]['state'],'incoming')
        self.assertFalse(await self.cloud.flush(),'No made-up or newly created EOS identities')
        self.bind()
        self.assertTrue(await self.cloud.flush())
        self.assertEqual(self.cloud.status(self.who)['state'],'synced')
        self.assertEqual(self.cloud.status(self.peer)['state'],'pending')
        record=json.dumps(self.uploads[0])
        for private in ('private-delegation',self.who,self.peer,'presence','token','name'):
            self.assertNotIn(private,record)
        self.assertIsNone(self.row()['payload'],'Confirmed payload is evicted from the outbox')

    async def test_consent_coalesces_block_replaces_and_rename_preserves_identity(self):
        a=await self.find(self.b,self.a);b=await self.find(self.a,self.b)
        await self.command(self.a,'request',target=b['id'])
        await self.command(self.b,'accept',target=a['id'])
        self.bind();await self.cloud.flush()
        self.assertEqual(self.uploads[-1][1]['relationships'][0]['state'],'friend')
        previous=self.row()['revision']
        self.a=await self.login('one','Renamed Pilot')
        await self.cloud.flush()
        self.assertEqual(self.row()['revision'],previous+1)
        self.assertEqual(self.uploads[-1][1]['player'],a['id'])
        self.assertRegex(self.uploads[-1][1]['friend_tag'],r'^Renamed Pilot#[0-9]{8}$')
        await self.command(self.a,'block',target=b['id'])
        await self.cloud.flush()
        self.assertEqual(self.uploads[-1][1]['relationships'][0]['state'],'blocked')
        self.assertEqual(json.loads(self.row(self.peer)['payload'])['relationships'],[])

    async def test_enqueue_rolls_back_with_transaction_and_unchanged_records_do_not_churn(self):
        self.bind();await self.cloud.flush()
        revision=self.row()['revision']
        with self.api.db:self.cloud.enqueue(self.who)
        self.assertEqual(self.row()['revision'],revision)
        try:
            with self.api.db:
                self.api.db.execute("UPDATE player_profiles SET screen_name='Rolled Back' WHERE player=?",(self.who,))
                self.cloud.enqueue(self.who)
                raise ValueError('rollback')
        except ValueError:pass
        self.assertEqual(self.row()['revision'],revision)
        self.assertFalse(await self.cloud.flush())

    async def test_presence_or_peer_rename_does_not_change_cloud_record(self):
        # UI order depends on names/presence; the persisted order uses public ID.
        b=await self.find(self.a,self.b);c=await self.find(self.a,self.c)
        await self.command(self.a,'request',target=b['id']);await self.command(self.a,'request',target=c['id'])
        self.bind();await self.cloud.flush();revision=self.row()['revision']
        await self.login('two','ZZZ');await self.login('three','AAA')
        with self.api.db:self.cloud.enqueue(self.who)
        self.assertEqual(self.row()['revision'],revision)

    async def test_failure_restart_and_retry_keep_pending_revision_without_private_errors(self):
        self.bind()
        async def fail(*_):raise ValueError('secret should never enter SQLite or API')
        self.cloud.upload=fail
        await self.cloud.flush()
        self.assertEqual(self.cloud.status(self.who)['state'],'unavailable')
        self.assertEqual(self.row()['failures'],1)
        self.assertNotIn('secret',self.row()['last_error'])
        self.assertFalse(await self.cloud.flush(),'Backoff prevents tight retry loops')
        from ..friend_cloud import FriendCloud
        restored=FriendCloud(self.api.directory,upload=lambda *_:None)
        restored.enabled=True
        async def ok(*_):pass
        restored.upload=ok
        with self.api.db:self.api.db.execute('UPDATE friend_cloud_outbox SET next_attempt=0')
        self.assertTrue(await restored.flush())
        self.assertEqual(restored.status(self.who)['state'],'synced')

    async def test_inflight_new_revision_or_binding_change_cannot_ack_current_state(self):
        self.bind()
        async def newer(*_):
            with self.api.db:
                self.api.db.execute("UPDATE player_profiles SET screen_name='Newer' WHERE player=?",(self.who,))
                self.cloud.enqueue(self.who)
        self.cloud.upload=newer;await self.cloud.flush()
        self.assertEqual(self.row()['revision'],2)
        self.assertEqual(self.row()['uploaded_revision'],0)
        async def rebind(*_):
            with self.api.db:self.api.db.execute('UPDATE eos_players SET puid=? WHERE player=?',('b'*32,self.who))
        self.cloud.upload=rebind;await self.cloud.flush()
        self.assertEqual(self.row()['uploaded_revision'],0)
        self.assertEqual(self.cloud.status(self.who)['state'],'unavailable')

    async def test_cancellation_leaves_outbox_retryable(self):
        self.bind();started=asyncio.Event()
        async def blocked(*_):started.set();await asyncio.Event().wait()
        self.cloud.upload=blocked
        task=asyncio.create_task(self.cloud.flush());await started.wait();task.cancel()
        with self.assertRaises(asyncio.CancelledError):await task
        self.assertEqual(self.row()['uploaded_revision'],0)
        self.assertIsNotNone(self.row()['payload'])

    async def test_worker_protocol_requires_exact_download_and_reaps_child(self):
        # A real child process exercises the private RPC; it cannot call EOS.
        script=Path(self.temp.name)/'worker.py'
        script.write_text('''import json,sys,time,os,pathlib
assert os.environ['H2_EOS_WORKER_MODE']=='friends'
assert not any(k in os.environ for k in ('H2_STEAM_KEY','STEAM_KEY','H2_EOS_SERVER_CONFIG','H2_EOS_FRIEND_CLIENT_SECRET','H2_MASTER_STATE','LD_PRELOAD'))
assert os.environ['HOME']==os.environ['H2_EOS_FRIEND_CACHE_DIR']
pathlib.Path(os.environ['H2_EOS_FRIEND_CACHE_DIR'],'private-cache').write_text('ephemeral')
r=json.loads(sys.stdin.readline())
assert r['op']=='friend_store'
assert r['token']=='private-token'
value={'record':r['record'] if sys.argv[-1]=='good' else 'wrong'}
print('H2EOS '+json.dumps({'ok':True,'value':value}),flush=True)
time.sleep(10)
''')
        self.cloud.binary=sys.executable;self.cloud.config=str(config_file(self.temp.name))
        create=asyncio.create_subprocess_exec
        async def child(binary,**kw):return await create(binary,str(script),'good',**kw)
        with (patch.dict(os.environ,dict(H2_STEAM_KEY='private-steam-key',H2_EOS_SERVER_CONFIG='/private/publisher',
                H2_EOS_FRIEND_CLIENT_SECRET='unrelated-override',H2_MASTER_STATE='/private/master',LD_PRELOAD='/private/inject.so')),
                patch('asyncio.create_subprocess_exec',side_effect=child)):
            await self.cloud.transfer('a'*32,'private-token','expected')
        self.assertIsNone(self.cloud.process)
        self.assertEqual(list((Path(self.temp.name)/'cache').iterdir()),[])
        async def wrong(binary,**kw):return await create(binary,str(script),'bad',**kw)
        with patch('asyncio.create_subprocess_exec',side_effect=wrong):
            with self.assertRaisesRegex(Exception,'read-back mismatch'):
                await self.cloud.transfer('a'*32,'private-token','expected')
        self.assertIsNone(self.cloud.process)
        self.assertEqual(list((Path(self.temp.name)/'cache').iterdir()),[])

    async def test_cancellation_during_launch_reaps_child_before_removing_cache(self):
        self.cloud.binary=sys.executable;self.cloud.config=str(config_file(self.temp.name))
        script=Path(self.temp.name)/'waiting.py';script.write_text('import time\ntime.sleep(30)\n')
        create=asyncio.create_subprocess_exec;spawned=asyncio.Event();release=asyncio.Event();children=[]
        async def delayed(binary,**kw):
            child=await create(binary,str(script),**kw);children.append(child);spawned.set()
            await release.wait()
            return child
        with patch('asyncio.create_subprocess_exec',side_effect=delayed):
            task=asyncio.create_task(self.cloud.transfer('a'*32,'private-token','expected'))
            await asyncio.wait_for(spawned.wait(),3);task.cancel();release.set()
            with self.assertRaises(asyncio.CancelledError):await task
        self.assertIsNotNone(children[0].returncode)
        self.assertIsNone(self.cloud.process)
        self.assertEqual(list((Path(self.temp.name)/'cache').iterdir()),[])
