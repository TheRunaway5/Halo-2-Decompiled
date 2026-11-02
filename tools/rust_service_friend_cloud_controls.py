"""Rust private friend-file IO controls. SDK/cloud replies are private fixtures."""
import asyncio
import base64
import hashlib
import json
import os
from pathlib import Path
import sqlite3
import sys
import time
import uuid


def qualify(binary,scratch,catalog,Server,room_only=False):
    scratch.mkdir();state=scratch/'state';config=scratch/'friend-storage.env'
    worker=scratch/'private-worker.py'
    control=scratch/'control.json';control.write_text('{}');calls=scratch/'calls.jsonl'
    worker.write_text('#!'+sys.executable+'\n'+'''import base64,hashlib,json,os,sys,time
from pathlib import Path
root=Path(@ROOT@)
if os.environ.get('H2_EOS_WORKER_MODE')!='friends':
    for line in sys.stdin:
        r=json.loads(line)
        assert r['op']=='verify'
        if r['token'].startswith('private-sdk-'):account=r['token'][12:]
        else:
            part=r['token'].split('.')[1];hint=json.loads(base64.urlsafe_b64decode(part+'='*(-len(part)%4)))
            assert hint['sub']==r['user'];account=hint['fixture_account']
        print('H2EOS '+json.dumps({'ok':True,'value':{'account':account}}),flush=True)
    sys.exit(0)
assert not any(k in os.environ for k in ('H2_STEAM_KEY','STEAM_KEY','H2_EOS_SERVER_CONFIG','H2_EOS_FRIEND_CLIENT_SECRET','H2_MASTER_STATE','LD_PRELOAD'))
cache=Path(os.environ['H2_EOS_FRIEND_CACHE_DIR'])
assert cache.is_dir() and cache.stat().st_mode&0o777==0o700
assert all(os.environ[k]==str(cache) for k in ('HOME','TMPDIR','TMP','TEMP','XDG_CACHE_HOME','XDG_CONFIG_HOME','XDG_DATA_HOME'))
(cache/'private-cache').write_text('ephemeral')
r=json.loads(sys.stdin.readline());assert set(r)=={'op','user','token','record'} and r['op']=='friend_store'
header,payload,signature=r['token'].split('.')
decode=lambda s:base64.urlsafe_b64decode(s+'='*(-len(s)%4))
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import padding,rsa
jwk=json.loads((root/'jwks.json').read_text())['keys'][0]
public=rsa.RSAPublicNumbers(int.from_bytes(decode(jwk['e']),'big'),int.from_bytes(decode(jwk['n']),'big')).public_key()
public.verify(decode(signature),(header+'.'+payload).encode(),padding.PKCS1v15(),hashes.SHA256())
claims=json.loads(decode(payload));settings=json.loads((root/'control.json').read_text())
assert json.loads(decode(header))['kid']==jwk['kid'] and json.loads(decode(header))['alg']=='RS256'
assert claims['sub']==settings['accounts'][r['user']] and claims['aud']=='rust-friend-cloud-fixture'
assert claims['iss']=='https://matchmaking.invalid' and claims['exp']-claims['iat']==120 and claims['iat']<=time.time()<claims['exp']
record=json.loads(r['record']);assert record['format']=='halo2-friends-v1'
assert claims['name']==record['friend_tag'].rsplit('#',1)[0]
row=dict(pid=os.getpid(),puid=r['user'],player=claims['sub'],revision=record['revision'],record=record,digest=hashlib.sha256(r['record'].encode()).hexdigest())
with (root/'calls.jsonl').open('a') as f:f.write(json.dumps(row)+'\\n')
mode=settings.get('mode','good')
if mode=='gate':
    while not (root/('release-'+claims['sub']+'-'+str(record['revision']))).exists():time.sleep(.005)
if mode=='timeout':time.sleep(140)
if mode=='duplicate':print('H2EOS {"ok":true,"ok":false}',flush=True)
elif mode=='oversize':print('x'*131073,flush=True)
elif mode=='lines':
    for _ in range(64):print('private fixture SDK log',flush=True)
    print('H2EOS '+json.dumps({'ok':True,'value':{'record':r['record']}}),flush=True)
else:print('H2EOS '+json.dumps({'ok':True,'value':{'record':'wrong' if mode=='wrong' else r['record']}}),flush=True)
time.sleep(30)
'''.replace('@ROOT@',repr(str(scratch))));worker.chmod(0o700)
    values=dict(H2_EOS_PRODUCT_ID='a'*32,H2_EOS_SANDBOX_ID='b'*32,H2_EOS_DEPLOYMENT_ID='c'*32,
        H2_EOS_FRIEND_CLIENT_ID='dedicated-fixture',H2_EOS_FRIEND_CLIENT_SECRET='private-fixture-secret',
        H2_EOS_FRIEND_CACHE_DIR=str(scratch/'cache'),H2_EOS_FRIEND_ENCRYPTION_KEY='d'*64)
    config.write_text('\n'.join(k+'='+v for k,v in values.items())+'\n');config.chmod(0o600)
    env=dict(H2_EOS_FRIEND_STORAGE='1',H2_EOS_FRIEND_STORAGE_CONFIG=str(config),H2_EOS_FRIEND_CACHE_DIR=str(scratch/'cache'),H2_EOS_WORKER=str(worker),
        H2_EOS_OPENID_AUDIENCE='rust-friend-cloud-fixture',H2_PUBLIC_URL='https://matchmaking.invalid',
        H2_STEAM_KEY='e'*32,STEAM_KEY='e'*32,H2_EOS_SERVER_CONFIG='/private/unused-publisher',
        H2_EOS_FRIEND_CLIENT_SECRET='unused-override',H2_MASTER_STATE='/private/unused-master',LD_PRELOAD='')
    server=None;checks=[];timings={};accounts={}
    def settings(mode='good'):
        temp=control.with_suffix('.tmp');temp.write_text(json.dumps(dict(mode=mode,accounts=accounts)));temp.replace(control)
    def events():return [json.loads(line) for line in calls.read_text().splitlines()] if calls.exists() else []
    def sql(text,params=()):
        with sqlite3.connect(state/'registry.sqlite3') as db:return db.execute(text,params).fetchall()
    def expect(method,path,status=200,**kw):
        actual,value=server.request(method,path,**kw);assert actual==status,(path,actual,status,value.get('error'));return value
    def wait(predicate,seconds=4):
        deadline=time.monotonic()+seconds
        while not predicate():
            assert time.monotonic()<deadline,'Private friend-file observation timed out';time.sleep(.03)
    def reaped():
        assert all(not Path('/proc',str(row['pid'])).exists() for row in events())
        assert not list((scratch/'cache').iterdir())
    try:
        original=config.read_text()
        for mode in ('public','symlink','duplicate','publisher','oversize','fifo','cache-public','cache-symlink'):
            config.write_text(original);config.chmod(0o600);bad=dict(env);extra=None
            if mode=='public':config.chmod(0o644)
            elif mode=='duplicate':config.write_text(original+original)
            elif mode=='publisher':config.write_text(original+'H2_EOS_SERVER_CLIENT_SECRET=unrelated\n')
            elif mode=='oversize':config.write_text('x'*16385)
            elif mode in ('symlink','fifo'):
                extra=scratch/('bad-'+mode)
                if mode=='symlink':extra.symlink_to(config)
                else:os.mkfifo(extra)
                bad['H2_EOS_FRIEND_STORAGE_CONFIG']=str(extra)
            else:
                extra=scratch/('bad-'+mode)
                if mode=='cache-public':extra.mkdir();extra.chmod(0o755)
                else:extra.symlink_to(scratch/'cache',target_is_directory=True)
                bad['H2_EOS_FRIEND_CACHE_DIR']=str(extra)
            try:
                unexpected=Server(binary,state,scratch,catalog,environment=bad,tls=True)
            except RuntimeError:pass
            else:unexpected.close();raise AssertionError('Invalid private friend configuration admitted: '+mode)
        config.write_text(original);config.chmod(0o600);settings()
        server=Server(binary,state,scratch,catalog,environment=env,tls=True)
        value=expect('GET','/health');assert 'friend_storage' in value['capabilities'] and value['ready'] is False
        checks.append('valid configured service starts; public/symlink/FIFO/duplicate/oversized/publisher config and public/symlink cache roots fail before listening')
        (scratch/'jwks.json').write_text(json.dumps(expect('GET','/v1/auth/eos/jwks')))
        if room_only:
            async def room_flow():
                import aiohttp
                entries=json.loads(catalog.read_text())['playlists'];maps=sorted({g['map'] for p in entries for g in p['games']});lab=next(p for p in entries if p['id']=='lab-test')
                raw_tokens=['h2s_'+c*43 for c in 'DE'];subjects=['76561197960287973','76561197960287974']
                with sqlite3.connect(state/'registry.sqlite3') as db:
                    with db:db.executemany('INSERT INTO steam_login_sessions VALUES(?,?,?,?)',[(hashlib.sha256(t.encode()).hexdigest(),s,'Private room cloud fixture',int(time.time())+3600) for t,s in zip(raw_tokens,subjects)])
                base=f'http://{server.host}:{server.port}';sockets=[];tasks=[]
                async with aiohttp.ClientSession() as client:
                    async def http(method,path,token,body=None,status=None):
                        async with client.request(method,base+path,headers={'Authorization':'Bearer '+token},json=body) as response:
                            result=await response.json();assert response.status in ((status,) if status else (200,201)),(path,response.status,result.get('error'));return result
                    async def read(ws):
                        async for message in ws:
                            if message.type!=aiohttp.WSMsgType.TEXT:break
                            row=json.loads(message.data)
                            if 'ping' in row:await ws.send_json({'pong':row['ping']})
                            elif 'probe' in row:await ws.send_json({'echo':row['probe']})
                    try:
                        grants=[(await http('POST','/v1/matchmaking/session',t,dict(screen_name='Queue Only '+str(i),maps=maps,compatibility='halo2-native-queue-v16',language=0)))['token'] for i,t in enumerate(raw_tokens)]
                        players=[sql('SELECT player FROM player_identities WHERE subject=?',(s,))[0][0] for s in subjects]
                        assert all(not sql('SELECT public_id FROM history_players WHERE player=?',(p,)) for p in players)
                        for t in grants:
                            ws=await client.ws_connect(base+'/v1/matchmaking/qos',headers={'Authorization':'Bearer '+t},compress=0);sockets.append(ws);tasks.append(asyncio.create_task(read(ws)))
                            await http('POST','/v1/matchmaking/ticket',t,dict(playlist_id=lab['id'],revision=lab['revision'],request_id=str(uuid.uuid4())))
                        deadline=time.monotonic()+20;view=None
                        while time.monotonic()<deadline:
                            view=await http('GET','/v1/matchmaking/ticket?progress=1',grants[0])
                            if view['match']:break
                            await asyncio.sleep(.05)
                        assert view and view['match']
                        descriptors=[await http('GET','/v1/matchmaking/transport',t) for t in grants]
                        for d in descriptors:
                            ws=await client.ws_connect(base+'/v1/matches/'+d['id']+'/signal',headers={'Authorization':'Bearer '+d['token']},compress=0);sockets.append(ws);tasks.append(asyncio.create_task(read(ws)))
                        d=descriptors[0]
                        assert not sql('SELECT public_id FROM history_players WHERE player=?',(players[0],))
                        sql('CREATE TABLE cloud_room_fail(player TEXT REFERENCES players(id) DEFERRABLE INITIALLY DEFERRED)')
                        sql("CREATE TRIGGER cloud_history_fail AFTER INSERT ON history_players BEGIN INSERT INTO cloud_room_fail VALUES('missing-private-player'); END")
                        hint=base64.urlsafe_b64encode(json.dumps({'sub':'e'*32,'fixture_account':players[0]}).encode()).decode().rstrip('=')
                        await http('POST','/v1/matches/'+d['id']+'/eos-proof',d['token'],{'token':'a.'+hint+'.private-fixture'},status=500)
                        assert not sql('SELECT public_id FROM history_players WHERE player=?',(players[0],))
                        assert not sql('SELECT puid FROM eos_players WHERE player=?',(players[0],)) and not events()
                        sql('DROP TRIGGER cloud_history_fail')
                        accounts['d'*32]=players[0];settings()
                        hint=base64.urlsafe_b64encode(json.dumps({'sub':'d'*32,'fixture_account':players[0]}).encode()).decode().rstrip('=')
                        await http('POST','/v1/matches/'+d['id']+'/eos-proof',d['token'],{'token':'a.'+hint+'.private-fixture'})
                        return players[0]
                    finally:
                        for ws in sockets:await ws.close()
                        await asyncio.gather(*tasks)
            player=asyncio.run(room_flow())
            wait(lambda:bool(sql('SELECT 1 FROM friend_cloud_outbox WHERE player=? AND uploaded_revision=revision AND payload IS NULL',(player,))))
            reaped();assert sql('SELECT public_id FROM history_players WHERE player=?',(player,))
            checks.append('actual source-v16 queue-only room EOS proof and private friend-file upload succeed without an account-session login or pre-existing public history ID')
            checks.append('a real deferred history COMMIT failure rolls back public identity, canonical PUID, outbox and native room proof; retry with a different verified PUID commits once')
            return dict(qualified=True,checks=checks,room_only=True,worker_calls=len(events()),worker_pids=[r['pid'] for r in events()],real_sdk=False,real_cloud=False,native_played_world=False)
        tokens=['h2s_'+c*43 for c in 'ABC'];subjects=[str(76561197960287970+i) for i in range(3)]
        with sqlite3.connect(state/'registry.sqlite3') as db:
            with db:db.executemany('INSERT INTO steam_login_sessions VALUES(?,?,?,?)',[(hashlib.sha256(t.encode()).hexdigest(),s,'Private cloud fixture',int(time.time())+3600) for t,s in zip(tokens,subjects)])
        def login(i,name):return expect('POST','/v1/account/session',201,token=tokens[i],body={'screen_name':name})['token']
        grants=[login(i,'Cloud Pilot '+str(i)) for i in range(3)]
        players=[sql('SELECT player FROM player_identities WHERE subject=?',(s,))[0][0] for s in subjects]
        public=[sql('SELECT public_id FROM history_players WHERE player=?',(p,))[0][0] for p in players]
        def row(i=0):return sql('SELECT revision,uploaded_revision,payload,failures,last_error,next_attempt FROM friend_cloud_outbox WHERE player=?',(players[i],))[0]
        def synced(i=0):return row(i)[0]==row(i)[1] and row(i)[2] is None
        def rename(name):grants[0]=login(0,name)
        def command(i,action,peer=1,**kw):return expect('POST','/v1/friends/commands',token=grants[i],body=dict(action=action,target=public[peer],request_id=str(uuid.uuid4()))|kw)
        command(0,'request');time.sleep(.15);assert not events()
        assert json.loads(row()[2])['relationships']==[dict(player=public[1],state='outgoing')]
        assert json.loads(row(1)[2])['relationships']==[dict(player=public[0],state='incoming')]
        checks.append('actual login and friend consent enqueue opaque public records for both peers; no synthetic EOS identity or upload is created for unbound accounts')
        accounts['a'*32]=players[0];settings()
        expect('POST','/v1/account/eos-proof',token=grants[0],body={'puid':'a'*32,'token':'private-sdk-'+players[0]})
        wait(synced);reaped();assert not synced(1)
        record=events()[-1]['record'];assert record['player']==public[0] and record['revision']==row()[0]
        assert all(secret not in json.dumps(record) for secret in (players[0],players[1],'private-sdk-','token','presence'))
        checks.append('real account EOS proof wakes upload; independently verified Rust RSA delegation binds the existing canonical account, issuer/audience and120-second lifetime; exact downloaded record evicts only the acknowledged payload')
        sql('CREATE TABLE cloud_fail(player TEXT REFERENCES players(id) DEFERRABLE INITIALLY DEFERRED)')
        accounts['c'*32]=players[2];settings();count=len(events());pending=row(2)
        sql("CREATE TRIGGER cloud_bind_fail AFTER INSERT ON eos_players BEGIN INSERT INTO cloud_fail VALUES('missing-private-player'); END")
        expect('POST','/v1/account/eos-proof',500,token=grants[2],body={'puid':'c'*32,'token':'private-sdk-'+players[2]})
        assert row(2)==pending and not sql('SELECT 1 FROM eos_players WHERE player=?',(players[2],)) and len(events())==count
        sql('DROP TRIGGER cloud_bind_fail')
        expect('POST','/v1/account/eos-proof',token=grants[2],body={'puid':'c'*32,'token':'private-sdk-'+players[2]})
        wait(lambda:synced(2));reaped()
        checks.append('actual failed EOS binding COMMIT publishes neither a canonical PUID nor an upload; verified retry binds and publishes its existing pending record once')
        sql("CREATE TRIGGER cloud_enqueue_fail AFTER UPDATE ON friend_cloud_outbox BEGIN INSERT INTO cloud_fail VALUES('missing-private-player'); END")
        before=[row(i) for i in range(3)];rid=str(uuid.uuid4())
        expect('POST','/v1/friends/commands',500,token=grants[0],body=dict(action='block',target=public[1],request_id=rid))
        assert before==[row(i) for i in range(3)] and not sql('SELECT 1 FROM player_friend_commands WHERE request=?',(rid,))
        sql('DROP TRIGGER cloud_enqueue_fail');command(0,'block',request_id=rid);wait(synced);count=len(events());revision=row()[0]
        command(0,'block',request_id=rid);time.sleep(.15);assert row()[0]==revision and len(events())==count
        checks.append('real deferred outbox COMMIT failure rolls back friendship, both payloads and replay; identical request retries once without upload or revision churn on committed replay')
        settings('gate');rename('Cloud Revision One');wait(lambda:len(events())>count);first=events()[-1]
        began=time.monotonic();expect('GET','/health');sql('CREATE TABLE independent_cloud_io(value INTEGER)');timings['health_and_sql_seconds']=time.monotonic()-began
        command(0,'unblock');new_revision=row()[0];assert new_revision>first['revision'];previous_upload=row()[1]
        (scratch/('release-'+players[0]+'-'+str(first['revision']))).write_text('ready')
        wait(lambda:events()[-1]['revision']==new_revision);assert row()[1]==previous_upload
        (scratch/('release-'+players[0]+'-'+str(new_revision))).write_text('ready');wait(synced);reaped()
        checks.append('health and an independent SQL write proceed during actual worker IO; a newer consent revision cannot be acknowledged by an older successful download')
        count=len(events());rename('Cloud Binding Check');wait(lambda:len(events())>count);stale=events()[-1]
        sql('UPDATE eos_players SET puid=? WHERE player=?',('b'*32,players[0]))
        (scratch/('release-'+players[0]+'-'+str(stale['revision']))).write_text('ready')
        wait(lambda:row()[3]==1);assert row()[0]>row()[1];reaped()
        sql('UPDATE eos_players SET puid=? WHERE player=?',('a'*32,players[0]))
        checks.append('a private SQL identity-change fixture during owned IO cannot acknowledge a successful reply under the former PUID')
        count=len(events());rename('Cloud Commit Retry');wait(lambda:len(events())>count);current=events()[-1]
        sql("CREATE TRIGGER cloud_finish_fail AFTER UPDATE ON friend_cloud_outbox WHEN NEW.uploaded_revision>OLD.uploaded_revision BEGIN INSERT INTO cloud_fail VALUES('missing-private-player'); END")
        (scratch/('release-'+players[0]+'-'+str(current['revision']))).write_text('ready')
        wait(lambda:not Path('/proc',str(current['pid'])).exists());time.sleep(.2);assert not synced()
        sql('DROP TRIGGER cloud_finish_fail');wait(synced,12);assert len(events())==count+1;reaped()
        checks.append('real deferred completion COMMIT failure retains the sealed claim; acknowledgement retry succeeds without repeating provider IO')
        for mode in ('wrong','duplicate','oversize','lines'):
            settings(mode);count=len(events());rename('Cloud Reject '+mode)
            wait(lambda:row()[3]==1);assert row()[4]=='unavailable' and row()[2] is not None and len(events())==count+1
            assert expect('GET','/v1/friends',token=grants[0])['cloud']['state']=='unavailable';reaped()
        settings();began=time.monotonic();count=len(events());wait(synced,42)
        timings['real_backoff_seconds']=time.monotonic()-began;assert timings['real_backoff_seconds']>=29 and len(events())==count+1;reaped()
        checks.append('wrong read-back, duplicate JSON, oversized lines and excess SDK log lines cannot acknowledge; native retry waits the unshortened thirty-second backoff and stores only a generic error')
        settings('timeout');count=len(events());rename('Cloud Real Timeout');wait(lambda:len(events())>count);began=time.monotonic()
        wait(lambda:row()[3]==1,134);timings['real_worker_timeout_seconds']=time.monotonic()-began
        assert 128<timings['real_worker_timeout_seconds']<134;reaped()
        checks.append('real130-second worker deadline kills/reaps the exact child before its private cache is removed and leaves the payload pending')
        settings('gate');count=len(events());rename('Cloud Shutdown');wait(lambda:len(events())>count);pending=row();pid=events()[-1]['pid']
        began=time.monotonic();server.close();timings['shutdown_seconds']=time.monotonic()-began;assert server.process.returncode==0;server=None
        assert timings['shutdown_seconds']<3 and not Path('/proc',str(pid)).exists() and not list((scratch/'cache').iterdir())
        assert row()==pending
        settings();server=Server(binary,state,scratch,catalog,environment=env,tls=True);wait(synced);reaped()
        checks.append('SIGTERM during real owned IO cancels and reaps before cache removal; restart retains the same pending revision and canonical EOS identity and uploads it through a fresh native claim')
        return dict(qualified=True,checks=checks,timings=timings,worker_calls=len(events()),worker_pids=[r['pid'] for r in events()],real_sdk=False,real_cloud=False,native_played_world=False)
    finally:
        if server:server.close()
