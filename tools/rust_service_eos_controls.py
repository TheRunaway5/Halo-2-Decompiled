"""Rust EOS identity HTTP/worker qualification, with private provider fixtures.

The worker is a declared SDK-receipt fixture. It cannot establish real Epic
authorization, a played world, original Xbox equivalence or delivered parity.
All files and child processes belong to the enclosing run_test.py owner.
"""
import base64
import asyncio
import hashlib
import http.client
import json
import os
from pathlib import Path
import sqlite3
import socket
import sys
import time
import uuid


def qualify(binary,scratch,catalog,Server):
    scratch.mkdir();state=scratch/'state';checks=[]
    control=scratch/'provider.json';control.write_text('{}')
    calls=scratch/'worker-calls.jsonl'
    worker=scratch/'sdk-receipt-fixture.py'
    worker.write_text('#!'+sys.executable+'\n'+'''import hashlib,json,os,sys,time
from pathlib import Path
control=Path(os.environ['H2_EOS_FIXTURE_CONTROL'])
calls=Path(os.environ['H2_EOS_FIXTURE_CALLS'])
for line in sys.stdin:
    request=json.loads(line)
    with calls.open('a') as output:
        output.write(json.dumps({'pid':os.getpid(),'digest':hashlib.sha256(request['token'].encode()).hexdigest()})+'\\n')
    row=json.loads(control.read_text()).get(request['token'])
    if row is not None:
        time.sleep(row.get('delay',0))
    if row is not None and row.get('mode')=='malformed':
        print('H2EOS {"ok":true,"ok":false}',flush=True)
    elif row is None or row.get('puid')!=request.get('user'):
        print('H2EOS '+json.dumps({'ok':False,'error':'Declared fixture rejection'}),flush=True)
    else:
        print('H2EOS '+json.dumps({'ok':True,'value':{'account':row['account']}}),flush=True)
''')
    worker.chmod(0o700)
    environment=dict(H2_EOS_OPENID_AUDIENCE='rust-eos-fixture-v1',H2_PUBLIC_URL='https://matchmaking.invalid',
        H2_EOS_WORKER=str(worker),H2_EOS_FIXTURE_CONTROL=str(control),H2_EOS_FIXTURE_CALLS=str(calls))
    server=Server(binary,state,scratch,catalog,environment=environment,tls=True)
    def expect(method,path,status,**kwargs):
        actual,value=server.request(method,path,**kwargs)
        assert actual==status,(method,path,actual,{'error':value.get('error'),'fields':sorted(value)})
        return value
    try:
        jwks=expect('GET','/v1/auth/eos/jwks',200)
        assert len(jwks['keys'])==1 and jwks['keys'][0]['alg']=='RS256'
        checks.append('actual Rust JWKS route, configured signing identity and private persistent key')
        key=state/'eos-openid-key.pem'
        assert key.stat().st_mode&0o777==0o600
        decode=lambda text:base64.urlsafe_b64decode(text+'='*(-len(text)%4))
        from cryptography.hazmat.primitives import hashes
        from cryptography.hazmat.primitives.asymmetric import padding,rsa
        public=rsa.RSAPublicNumbers(int.from_bytes(decode(jwks['keys'][0]['e']),'big'),
            int.from_bytes(decode(jwks['keys'][0]['n']),'big')).public_key()
        tokens=['h2s_'+c*43 for c in 'ABCDEF']
        subjects=['765611979602879'+str(i) for i in range(80,86)]
        with sqlite3.connect(state/'registry.sqlite3') as db:
            db.executemany('INSERT INTO steam_login_sessions VALUES(?,?,?,?)',[
                (hashlib.sha256(t.encode()).hexdigest(),subject,'EOS Fixture',int(time.time())+3600)
                for t,subject in zip(tokens,subjects)])
        grants=[expect('POST','/v1/account/session',201,token=t,body={'screen_name':'Rust EOS '+str(i)})['token'] for i,t in enumerate(tokens)]
        with sqlite3.connect(state/'registry.sqlite3') as db:
            players=[db.execute("SELECT player FROM player_identities WHERE provider='steam' AND subject=?",[s]).fetchone()[0] for s in subjects]
        issued=expect('POST','/v1/account/eos-token',200,token=grants[0],body={})
        header,payload,signature=issued['token'].split('.')
        public.verify(decode(signature),(header+'.'+payload).encode(),padding.PKCS1v15(),hashes.SHA256())
        claims=json.loads(decode(payload))
        assert claims['sub']==players[0] and claims['aud']==environment['H2_EOS_OPENID_AUDIENCE']
        assert claims['iss']==environment['H2_PUBLIC_URL'] and claims['name']=='Rust EOS 0'
        assert claims['exp']-claims['iat']==120 and claims['nbf']==claims['iat']-5 and issued['expires_in']==120
        assert json.loads(decode(header))['kid']==jwks['keys'][0]['kid']
        expect('POST','/v1/account/eos-token',401,body={})
        expect('POST','/v1/account/eos-token',400,token=grants[0],body={'player':players[1]})
        checks.append('actual RS256 signature verifies against the public JWK; canonical account/name, issuer/audience and full120-second lifetime are bound without caller identity')
        registry={}
        def proof(puid,label):
            return 'a.'+base64.urlsafe_b64encode(json.dumps({'sub':puid}).encode()).decode().rstrip('=')+'.'+label
        def register(puid,player,label,**extra):
            token=proof(puid,label);registry[token]=dict(puid=puid,account=player,**extra)
            temp=control.with_suffix('.tmp');temp.write_text(json.dumps(registry));temp.replace(control)
            return token
        def sql(statement,params=()):
            with sqlite3.connect(state/'registry.sqlite3') as db:return db.execute(statement,params).fetchall()
        def count_calls():return len(calls.read_text().splitlines()) if calls.exists() else 0
        good=register('1'*32,players[0],'good')
        wrong=register('1'*32,players[1],'wrong-account')
        expect('POST','/v1/account/eos-proof',403,token=grants[0],body={'puid':'1'*32,'token':wrong})
        assert sql('SELECT count(*) FROM eos_players')[0][0]==0
        old_revision=sql('SELECT revision FROM eos_profile_outbox WHERE player=?',[players[0]])
        with sqlite3.connect(state/'registry.sqlite3') as db:
            db.executescript("CREATE TABLE eos_parent(id TEXT PRIMARY KEY);CREATE TABLE eos_failure(id TEXT REFERENCES eos_parent(id) DEFERRABLE INITIALLY DEFERRED);CREATE TRIGGER eos_fail AFTER INSERT ON eos_players BEGIN INSERT INTO eos_failure VALUES('missing');END;")
        expect('POST','/v1/account/eos-proof',500,token=grants[0],body={'puid':'1'*32,'token':good})
        assert sql('SELECT count(*) FROM eos_players')[0][0]==0
        assert sql('SELECT revision FROM eos_profile_outbox WHERE player=?',[players[0]])==old_revision
        sql('DROP TRIGGER eos_fail')
        expect('POST','/v1/account/eos-proof',200,token=grants[0],body={'puid':'1'*32,'token':good})
        revision=sql('SELECT revision FROM eos_profile_outbox WHERE player=?',[players[0]])
        expect('POST','/v1/account/eos-proof',200,token=grants[0],body={'puid':'1'*32,'token':good})
        assert sql('SELECT revision FROM eos_profile_outbox WHERE player=?',[players[0]])==revision
        changed=register('2'*32,players[0],'changed')
        expect('POST','/v1/account/eos-proof',409,token=grants[0],body={'puid':'2'*32,'token':changed})
        reused=register('1'*32,players[1],'reused')
        expect('POST','/v1/account/eos-proof',409,token=grants[1],body={'puid':'1'*32,'token':reused})
        checks.append('SDK-receipt account check, immutable unique PUID binding, actual failed COMMIT rollback and exactly-once outbox revision/retry')
        paused=register('3'*32,players[2],'paused-body')
        body=json.dumps({'puid':'3'*32,'token':paused}).encode();before=count_calls()
        with socket.create_connection((server.host,server.port),timeout=5) as wire:
            wire.sendall((f'POST /v1/account/eos-proof HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/json\r\nAuthorization: Bearer {grants[2]}\r\nContent-Length: {len(body)}\r\n\r\n').encode()+body[:1])
            time.sleep(.1)
            expect('DELETE','/v1/account/session',200,token=grants[2])
            wire.sendall(body[1:]);reply=http.client.HTTPResponse(wire);reply.begin();data=json.loads(reply.read())
            assert reply.status==401,('revoked body',reply.status,data.get('error'))
        assert count_calls()==before,'A revoked body upload reached the SDK verifier'
        assert not sql('SELECT puid FROM eos_players WHERE player=?',[players[2]])
        grants[2]=expect('POST','/v1/account/session',201,token=tokens[2],body={'screen_name':'Rust EOS 2'})['token']
        checks.append('revocation during actual body upload is rechecked before SDK IO and cannot persist a link')
        observed=asyncio.run(provider_controls(server,state,grants,players,register,calls))
        checks.extend(observed.pop('checks'))
        rooms=asyncio.run(room_controls(server,state,catalog,tokens,players,grants,register,calls,public))
        checks.extend(rooms.pop('checks'))
        key_hash=hashlib.sha256(key.read_bytes()).hexdigest();old=grants[0]
        grants[5]=expect('POST','/v1/account/session',201,token=tokens[5],body={'screen_name':'Rust EOS 5'})['token']
        shutdown_token=register('b'*32,players[5],'shutdown',delay=30)
        shutdown=asyncio.run(shutdown_control(server,grants[5],shutdown_token,calls))
        assert not sql('SELECT puid FROM eos_players WHERE player=?',[players[5]])
        checks.append('actual SIGTERM during owned SDK IO cancels and reaps the exact verifier child before process exit, without linking the pending identity')
        server=Server(binary,state,scratch,catalog,environment=environment,tls=True)
        assert expect('GET','/v1/auth/eos/jwks',200)==jwks
        assert hashlib.sha256(key.read_bytes()).hexdigest()==key_hash
        expect('POST','/v1/account/eos-token',401,token=old,body={})
        fresh=expect('POST','/v1/account/session',201,token=tokens[0],body={'screen_name':'Rust EOS 0'})['token']
        expect('POST','/v1/account/eos-proof',200,token=fresh,body={'puid':'1'*32,'token':good})
        assert sql('SELECT revision FROM eos_profile_outbox WHERE player=?',[players[0]])==revision
        assert expect('GET','/health',200)['ready'] is False
        checks.append('real Rust process restart retains the signing key, canonical PUID and revision while retiring old capabilities; full readiness remains false')
        return dict(checks=checks,provider=observed,rooms=rooms,shutdown=shutdown,public_key_kid=jwks['keys'][0]['kid'],
            scope='Real Rust EOS HTTP, RSA cryptography, SQL and owned worker IO; private SDK receipt/provider fixtures only. No real Epic authorization, native peer world, original or delivered acceptance.')
    finally:
        server.close()


async def shutdown_control(server,grant,token,calls):
    import aiohttp
    prior=len(calls.read_text().splitlines())
    async with aiohttp.ClientSession() as client:
        async def request():
            try:
                async with client.post(f'http://{server.host}:{server.port}/v1/account/eos-proof',
                    headers={'Authorization':'Bearer '+grant},json={'puid':'b'*32,'token':token}) as response:
                    await response.read();assert response.status==503,response.status
            except (aiohttp.ClientError,ConnectionError):pass
        task=asyncio.create_task(request());deadline=time.monotonic()+3
        while len(calls.read_text().splitlines())==prior and time.monotonic()<deadline:await asyncio.sleep(.01)
        assert len(calls.read_text().splitlines())>prior
        child=json.loads(calls.read_text().splitlines()[-1])['pid'];started=time.monotonic()
        await asyncio.to_thread(server.close);await task;elapsed=time.monotonic()-started
        assert elapsed<5 and server.process.returncode==0,(elapsed,server.process.returncode)
        if sys.platform.startswith('linux'):assert not Path('/proc',str(child)).exists()
        return dict(elapsed_seconds=elapsed,worker_pid=child,child_reaped=True,service_exit_code=server.process.returncode)


async def provider_controls(server,state,grants,players,register,calls):
    import aiohttp
    checks=[];base=f'http://{server.host}:{server.port}'
    def rows():return [json.loads(line) for line in calls.read_text().splitlines()] if calls.exists() else []
    async def called(token,prior):
        digest=hashlib.sha256(token.encode()).hexdigest();deadline=time.monotonic()+3
        while time.monotonic()<deadline:
            if any(r['digest']==digest for r in rows()[prior:]):return
            await asyncio.sleep(.01)
        raise AssertionError('Owned fixture worker did not receive the expected request')
    async with aiohttp.ClientSession(timeout=aiohttp.ClientTimeout(total=27)) as client:
        async def proof(index,puid,token,status):
            async with client.post(base+'/v1/account/eos-proof',headers={'Authorization':'Bearer '+grants[index]},json={'puid':puid,'token':token}) as response:
                value=await response.json();assert response.status==status,('provider control',response.status,status,value.get('error'))
                return value
        delayed=register('4'*32,players[3],'delayed',delay=3)
        before=len(rows());active=asyncio.create_task(proof(3,'4'*32,delayed,200));await called(delayed,before)
        await proof(3,'4'*32,delayed,429)
        queued=register('5'*32,players[4],'queued')
        await proof(4,'5'*32,queued,503)
        assert len(rows())==before+1,'A queue timeout disturbed the active worker stream'
        await active
        checks.append('actual three-second SDK IO survives a distinct two-second queue timeout; duplicate in-flight principal is rejected without consuming another reply')
        late=register('6'*32,players[5],'late-account',delay=.6)
        before=len(rows());active=asyncio.create_task(proof(5,'6'*32,late,401));await called(late,before)
        async with client.delete(base+'/v1/account/session',headers={'Authorization':'Bearer '+grants[5]}) as response:assert response.status==200
        await active
        with sqlite3.connect(state/'registry.sqlite3') as db:assert not db.execute('SELECT puid FROM eos_players WHERE player=?',[players[5]]).fetchall()
        checks.append('account revocation while the actual worker owns a proof is rechecked after SDK IO and persists no identity')
        malformed=register('5'*32,players[4],'malformed',mode='malformed')
        before=len(rows());await proof(4,'5'*32,malformed,503);bad_pid=rows()[-1]['pid']
        recovery=register('5'*32,players[4],'recovery')
        await proof(4,'5'*32,recovery,200);fresh_pid=rows()[-1]['pid']
        assert bad_pid!=fresh_pid
        if sys.platform.startswith('linux'):assert not Path('/proc',str(bad_pid)).exists()
        checks.append('duplicate JSON from the actual worker retires/reaps that exact stream and a fresh process verifies the next account')
        timeout_token=register('5'*32,players[4],'timeout',delay=30)
        before=len(rows());started=time.monotonic();await proof(4,'5'*32,timeout_token,503)
        elapsed=time.monotonic()-started;expired_pid=rows()[-1]['pid']
        assert 19<=elapsed<25,elapsed
        await proof(4,'5'*32,recovery,200)
        assert rows()[-1]['pid']!=expired_pid
        if sys.platform.startswith('linux'):assert not Path('/proc',str(expired_pid)).exists()
        checks.append('real twenty-second verifier deadline kills/reaps its owned child; no stale reply reaches the following request')
        return dict(checks=checks,queue_timeout_seconds=2,owned_timeout_seconds=elapsed,
            retired_worker_pids=[bad_pid,expired_pid],real_sdk=False)


async def room_controls(server,state,catalog,tokens,players,account_grants,register,calls,public):
    import aiohttp
    from cryptography.hazmat.primitives import hashes
    from cryptography.hazmat.primitives.asymmetric import padding
    base=f'http://{server.host}:{server.port}';checks=[];entries=json.loads(catalog.read_text())['playlists']
    maps=sorted({g['map'] for p in entries for g in p['games']});lab=next(p for p in entries if p['id']=='lab-test')
    sockets=[];tasks=[]
    def sql(statement,params=()):
        with sqlite3.connect(state/'registry.sqlite3') as db:return db.execute(statement,params).fetchall()
    async with aiohttp.ClientSession() as client:
        async def http(method,path,status,token=None,body=None):
            async with client.request(method,base+path,headers={'Authorization':'Bearer '+token} if token else {},json=body) as response:
                value=await response.json();assert response.status==status,(path,response.status,status,value.get('error'));return value
        async def reader(ws,queue=None):
            async for message in ws:
                if message.type!=aiohttp.WSMsgType.TEXT:break
                value=json.loads(message.data)
                if 'ping' in value:await ws.send_json({'pong':value['ping']})
                elif 'probe' in value:await ws.send_json({'echo':value['probe']})
                elif queue is not None:await queue.put(value)
            if queue is not None:await queue.put(None)
        async def signal(queue,timeout=2):
            deadline=time.monotonic()+timeout
            while time.monotonic()<deadline:
                try:row=await asyncio.wait_for(queue.get(),deadline-time.monotonic())
                except TimeoutError:return None
                if row is None:return None
                if 'peer_signal' in row:return row['peer_signal']
            return None
        try:
            grants=[(await http('POST','/v1/matchmaking/session',201,tokens[i],dict(screen_name='EOS Room '+str(i),
                maps=maps,compatibility='halo2-native-queue-v16',language=0,local_players=[{'screen_name':'EOS Guest'}] if i==1 else [])))['token'] for i in (1,2)]
            for grant in grants:
                ws=await client.ws_connect(base+'/v1/matchmaking/qos',headers={'Authorization':'Bearer '+grant},compress=0)
                sockets.append(ws);tasks.append(asyncio.create_task(reader(ws)))
                await http('POST','/v1/matchmaking/ticket',201,grant,dict(playlist_id=lab['id'],revision=lab['revision'],request_id=str(uuid.uuid4())))
            deadline=time.monotonic()+20
            while time.monotonic()<deadline:
                view=await http('GET','/v1/matchmaking/ticket?progress=1',200,grants[0])
                if view['match']:break
                await asyncio.sleep(.05)
            assert view['match'],'Source-v16 EOS room proposal unavailable'
            descriptors=[await http('GET','/v1/matchmaking/transport',200,t) for t in grants]
            assert all(d['transport']['kind']=='eos-p2p-v1' for d in descriptors)
            assert descriptors[0]['id']==descriptors[1]['id'] and len(descriptors[0]['players'])==3
            route=lambda d,suffix:'/v1/matches/'+d['id']+'/'+suffix
            d=descriptors[0];cap=d['token'];puids=['7'*32,'8'*32]
            await http('POST',route(d,'eos-token'),401,grants[0],{})
            await http('POST',route(d,'eos-token'),400,cap,{'player':players[2]})
            token=(await http('POST',route(d,'eos-token'),200,cap,{}))['token'];a,b,c=token.split('.')
            public.verify(base64.urlsafe_b64decode(c+'='*(-len(c)%4)),(a+'.'+b).encode(),padding.PKCS1v15(),hashes.SHA256())
            assert json.loads(base64.urlsafe_b64decode(b+'='*(-len(b)%4)))['sub']==players[1]
            before=len(calls.read_text().splitlines())
            duplicate=base64.urlsafe_b64encode(b'{"sub":"77777777777777777777777777777777","sub":"88888888888888888888888888888888"}').decode().rstrip('=')
            for bad in ('opaque','a..b','a.%%%.b','a.'+duplicate+'.b'):
                await http('POST',route(d,'eos-proof'),400,cap,{'token':bad})
            assert len(calls.read_text().splitlines())==before
            # The failed transaction uses a different PUID. A subsequent
            # successful proof would conflict if native proof state leaked.
            proof=register('9'*32,players[1],'room-rejected-commit')
            with sqlite3.connect(state/'registry.sqlite3') as db:
                db.executescript("CREATE TRIGGER eos_fail_room AFTER INSERT ON eos_players BEGIN INSERT INTO eos_failure VALUES('missing');END;")
            await http('POST',route(d,'eos-proof'),500,cap,{'token':proof})
            assert not sql('SELECT puid FROM eos_players WHERE player=?',[players[1]])
            streams=[];queues=[]
            for row in descriptors:
                ws=await client.ws_connect(base+route(row,'signal'),headers={'Authorization':'Bearer '+row['token']},compress=0)
                queue=asyncio.Queue();sockets.append(ws);streams.append(ws);queues.append(queue);tasks.append(asyncio.create_task(reader(ws,queue)))
            sql('DROP TRIGGER eos_fail_room')
            for i,row in enumerate(descriptors):
                proof=register(puids[i],players[i+1],'room-'+str(i))
                await http('POST',route(row,'eos-proof'),200,row['token'],{'token':proof})
            for i in (0,1):
                await streams[i].send_json({'peer_signal':{'peer':descriptors[1-i]['slot'],'type':'eos','payload':puids[i],'mid':''}})
                value=await signal(queues[1-i]);assert value and value['payload']==puids[i] and value['peer']==descriptors[i]['slot']
            await http('DELETE','/v1/account/session',200,account_grants[1])
            await http('POST',route(d,'eos-token'),200,cap,{})
            checks.append('source-v16 full-options room grants issue real signed EOS tokens independent of account sessions; malformed hints never reach the verifier')
            checks.append('failed room-proof COMMIT rolls back canonical and native proof publication; retry binds only SDK-verified PUIDs to real signal sockets and rejects forged identities')
            late=register(puids[1],players[2],'room-late',delay=.5)
            prior=len(calls.read_text().splitlines());pending=asyncio.create_task(http('POST',route(d,'eos-proof'),404,descriptors[1]['token'],{'token':late}))
            deadline=time.monotonic()+3
            while len(calls.read_text().splitlines())==prior and time.monotonic()<deadline:await asyncio.sleep(.01)
            assert len(calls.read_text().splitlines())>prior
            await streams[0].send_json({'peer_signal':{'peer':descriptors[1]['slot'],'type':'eos','payload':'f'*32,'mid':''}})
            assert await signal(queues[1],.5) is None,'Forged peer identity was delivered'
            await pending
            checks.append('a forged peer identity is rejected and closes the actual room; SDK IO cannot publish a late proof after that teardown')
            return dict(checks=checks,compatibility='halo2-native-queue-v16',physical_owners=2,logical_seats=3,
                real_signed_tokens=True,real_signal_sockets=True,real_sdk=False,played_world=False)
        finally:
            for ws in sockets:await ws.close()
            await asyncio.gather(*tasks)
