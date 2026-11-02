"""Private HTTP and WebSocket checks for the shared Rust voice contracts."""
import asyncio
import hashlib
import json
import os
from pathlib import Path
import sqlite3
import subprocess
import time
import uuid


def preflight(binary, scratch, catalog, Server):
    scratch.mkdir()
    server = Server(binary, scratch / 'state', scratch, catalog)
    checks = []
    try:
        for method, path, body in (
            ('PUT', '/v1/account/voice-blocks', {'blocked': []}),
            ('GET', '/v1/clans/voice', None),
            ('GET', '/v1/party/voice', None),
        ):
            status, value = server.request(method, path, body=body)
            assert status == 401, (method, path, status, value.get('error'))
            checks.append(path + ' requires authorization before voice IO')
        return dict(authentication_preflight_passed=True, checks=checks,
                    relay_qualified=False, real_audio=False, original_parity=False)
    finally:
        server.close()


def qualify(binary,scratch,catalog,Server,native_voice_probe=None):
    scratch.mkdir();checks=[];timings={};native=[];state=scratch/'state';server=None
    preflight(binary,scratch/'authorization',catalog,Server)
    checks.append('actual Rust HTTP denies anonymous preference and clan/party voice requests before any media upgrade')
    server=Server(binary,state,scratch,catalog)
    def expect(method,path,status=200,**kw):
        actual,value=server.request(method,path,**kw);assert actual==status,(path,actual,status,value.get('error'));return value
    def sql(text,params=()):
        with sqlite3.connect(state/'registry.sqlite3') as db:return db.execute(text,params).fetchall()
    tokens=['h2s_'+c*43 for c in 'ABCDE'];subjects=[str(76561197960287940+i) for i in range(5)]
    aliases=[str(i+1)*18 for i in range(4)];names=['voice_'+str(i) for i in range(4)]
    def login(i):return expect('POST','/v1/account/session',201,token=tokens[i],body={'screen_name':'Voice Pilot '+str(i)})['token']
    def prefs(i,blocked,status=200,**extra):return expect('PUT','/v1/account/voice-blocks',status,token=tokens[i],body=dict(blocked=blocked)|extra)
    try:
        with sqlite3.connect(state/'registry.sqlite3') as db:
            with db:db.executemany('INSERT INTO steam_login_sessions VALUES(?,?,?,?)',[(hashlib.sha256(t.encode()).hexdigest(),s,'Voice fixture',int(time.time())+3600) for t,s in zip(tokens,subjects)])
        prefs(4,[],409)
        grants=[login(i) for i in range(4)]
        players=[sql('SELECT player FROM player_identities WHERE subject=?',(s,))[0][0] for s in subjects[:4]]
        with sqlite3.connect(state/'registry.sqlite3') as db:
            with db:
                db.executemany("INSERT INTO player_identities VALUES('discord',?,?)",zip(aliases,players))
                db.executemany('UPDATE player_profiles SET discord_username=? WHERE player=?',zip(names,players))
        prefs(0,[aliases[0],aliases[1],'999999999999999999'])
        assert sql('SELECT subject FROM voice_blocks WHERE player=? ORDER BY subject',(players[0],))==[(aliases[1],),('999999999999999999',)]
        for body in ({'blocked':[aliases[1]],'player':'forged'},{'blocked':[1]},{'blocked':['invalid']},{'blocked':[aliases[1]]*2}):
            expect('PUT','/v1/account/voice-blocks',400,token=tokens[0],body=body)
        for method in ('GET','DELETE'):expect(method,'/v1/account/voice-blocks',405,token=tokens[0])
        large=[str(900000000000000000+i) for i in range(2000)]
        prefs(0,large);assert sql('SELECT count(*) FROM voice_blocks WHERE player=?',(players[0],))[0][0]==2000
        prefs(0,large+['999999999999999998'],400)
        expect('PUT','/v1/account/voice-blocks',413,token=tokens[0],raw=b'{"blocked":['+b'"123456789012345678",'*3000+b'"111111111111111111"]}')
        checks.append('private provider-bound replacement removes self blocks and rejects forged fields, duplicates, invalid IDs and oversized bodies; the full2000-subject record crosses the ordinary16KiB limit')
        sql('CREATE TABLE voice_failure(player TEXT REFERENCES players(id) DEFERRABLE INITIALLY DEFERRED)')
        sql("CREATE TRIGGER fail_voice_preferences AFTER DELETE ON voice_blocks BEGIN INSERT INTO voice_failure VALUES('missing-private-player'); END")
        prefs(0,[aliases[1]],500);assert sql('SELECT count(*) FROM voice_blocks WHERE player=?',(players[0],))[0][0]==2000
        sql('DROP TRIGGER fail_voice_preferences');prefs(0,[aliases[1]])
        checks.append('actual deferred preference COMMIT failure retains the complete old snapshot; retry replaces only the verified canonical owner')
        async def sql_wait_expiry():
            import aiohttp
            with sqlite3.connect(state/'registry.sqlite3') as db:
                db.execute('BEGIN IMMEDIATE')
                async with aiohttp.ClientSession() as client:
                    async def send():
                        async with client.put(f'http://{server.host}:{server.port}/v1/account/voice-blocks',headers={'Authorization':'Bearer '+tokens[0]},json={'blocked':[]}) as response:
                            await response.read();assert response.status==401,response.status
                    task=asyncio.create_task(send());await asyncio.sleep(.25)
                    db.execute('UPDATE steam_login_sessions SET expires=? WHERE token_hash=?',(int(time.time())-1,hashlib.sha256(tokens[0].encode()).hexdigest()));db.commit();await task
        asyncio.run(sql_wait_expiry());assert sql('SELECT subject FROM voice_blocks WHERE player=?',(players[0],))==[(aliases[1],)]
        sql('UPDATE steam_login_sessions SET expires=? WHERE token_hash=?',(int(time.time())+3600,hashlib.sha256(tokens[0].encode()).hexdigest()))
        checks.append('Steam proof expiry after the actual SQLite write lock is re-read before preference mutation and cannot consume the current snapshot')
        server.close();server=Server(binary,state,scratch,catalog);old=grants[0];grants=[login(i) for i in range(4)]
        assert sql('SELECT subject FROM voice_blocks WHERE player=?',(players[0],))==[(aliases[1],)]
        checks.append('real Rust process restart retains private preferences and canonical aliases while retiring old account grants')
        def party(i):return expect('GET','/v1/party',token=grants[i])
        def pc(i,action,**fields):
            body=dict(action=action,request_id=str(uuid.uuid4()))|fields
            if action not in ('create','accept') and 'version' not in body:body['version']=party(i)['party']['version']
            return expect('POST','/v1/party/commands',token=grants[i],body=body)
        def clan(i):return expect('GET','/v1/clans',token=grants[i])
        def cc(i,action,**fields):return expect('POST','/v1/clans/commands',token=grants[i],body=dict(action=action,request_id=str(uuid.uuid4()))|fields)
        async def media():
            import aiohttp
            from aiohttp import WSMsgType,WSServerHandshakeError
            base=f'http://{server.host}:{server.port}';streams=[];sequence={}
            async with aiohttp.ClientSession() as client:
                async def ws(kind,i,token=None,status=None,autoping=True):
                    try:value=await client.ws_connect(base+('/v1/party/voice' if kind=='party' else '/v1/clans/voice'),headers={'Authorization':'Bearer '+(token or grants[i])},compress=0,autoping=autoping)
                    except WSServerHandshakeError as e:assert status==e.status,(kind,e.status,status);return
                    assert status is None;streams.append(value);return value
                async def frame(wire,raw=None):
                    n=sequence.get(id(wire),0)+1;sequence[id(wire)]=n
                    packet=raw or b'\x01'+n.to_bytes(4,'big')+b'\x78\0';await wire.send_bytes(packet);return packet
                async def received(wire,packet,identity=None):
                    row=await wire.receive(timeout=2);assert row.type==WSMsgType.BINARY,(row.type,wire.close_code)
                    assert row.data[:1]==b'\x01' and row.data[17:]==packet[1:]
                    if identity:assert row.data[1:17]==uuid.UUID(identity).bytes
                async def quiet(wire):
                    try:row=await wire.receive(timeout=.12)
                    except asyncio.TimeoutError:return
                    raise AssertionError(('Unexpected cross-scope or blocked voice',row.type,wire.close_code))
                async def closed(wire,allowed=(1008,),timeout=2,label='membership retired'):
                    for _ in range(64):
                        row=await wire.receive(timeout=timeout)
                        if row.type in (WSMsgType.CLOSE,WSMsgType.CLOSED,WSMsgType.ERROR):
                            if row.type==WSMsgType.CLOSE:assert row.data in allowed,(label,'wire close',row.data,allowed)
                            assert wire.close_code in allowed,(label,wire.close_code,allowed,row.type,repr(row.data));return
                    raise AssertionError('Voice stream did not retire')
                try:
                    await ws('party',0,status=403);await ws('clan',0,status=403);await ws('party',0,old,status=401);await ws('party',0,tokens[0],status=401)
                    pc(0,'create');pc(0,'invite',username=names[1]);await ws('party',1,status=403)
                    invite=party(1)['invitations'][0];pc(1,'accept',target=invite['id'],version=invite['version'])
                    pc(0,'invite',username=names[2]);invite=party(2)['invitations'][0];pc(2,'accept',target=invite['id'],version=invite['version']);pc(3,'create')
                    prefs(0,[])
                    a,b,c,d=[await ws('party',i) for i in range(4)]
                    member=party(0)['party']['members'];handles={i:next(m['handle'] for m in member if m['name']=='Voice Pilot '+str(i)) for i in range(3)}
                    packet=await frame(a);await received(b,packet,handles[0]);await received(c,packet,handles[0]);await quiet(d)
                    checks.append('real party voice requires accepted membership and a current account grant; emitted audio carries the accepted opaque handle and never reaches another party')
                    prefs(0,[aliases[1]]);packet=await frame(a);await received(c,packet);await quiet(b)
                    packet=await frame(b);await received(c,packet);await quiet(a);prefs(1,[])
                    packet=await frame(b);await received(c,packet);await quiet(a);prefs(0,[])
                    packet=await frame(a);await received(b,packet);await received(c,packet)
                    checks.append('actual provider-bound Discord preference snapshots silence both directions while preserving third-party delivery; another participant cannot clear the blocker')
                    cc(0,'create',name='Rust Voice');cc(0,'invite',username=names[1]);invite=clan(1)['invitations'][0];cc(1,'accept',target=invite['id'])
                    cc(0,'invite',username=names[2]);invite=clan(2)['invitations'][0];cc(2,'accept',target=invite['id']);cc(3,'create',name='Other Voice')
                    ca,cb,ccs,cd=[await ws('clan',i) for i in range(4)]
                    packet=await frame(ca);await received(cb,packet,players[0]);await received(ccs,packet,players[0]);await quiet(cd)
                    packet=await frame(a);await received(b,packet);await received(c,packet);await quiet(cb)
                    checks.append('clan and party channels use independent native registries, memberships, sequence streams and audiences')
                    pc(0,'transfer',target=handles[1]);packet=await frame(b);await received(a,packet);await received(c,packet)
                    pc(1,'remove',target=handles[0]);await closed(a)
                    packet=await frame(ca);await received(cb,packet);await received(ccs,packet)
                    with sqlite3.connect(state/'registry.sqlite3') as db:
                        with db:
                            saved=db.execute('SELECT * FROM clan_members WHERE player=?',(players[2],)).fetchone();db.execute('DELETE FROM clan_members WHERE player=?',(players[2],));db.execute('INSERT INTO clan_members VALUES('+','.join('?' for _ in saved)+')',saved)
                    await closed(ccs);ccs=await ws('clan',2)
                    cc(0,'remove',target=players[2]);await closed(ccs);await ws('clan',2,status=403)
                    packet=await frame(b);await received(c,packet)
                    checks.append('leader transfer preserves accepted streams; removal and same-identity SQL membership reincarnation retire only the old native channel incarnation')
                    current=await ws('clan',0);await closed(ca,label='replaced clan0');await current.send_str('identity spoof');await closed(current,(1003,1008),label='nonbinary input')
                    for packets in ((b'\x02\0\0\0\0x',),(b'x'*262,),(b'\x01\0\0\0\1x',)*2,tuple(b'\x01'+i.to_bytes(4,'big')+b'x' for i in range(20))):
                        current=await ws('clan',0)
                        for packet in packets:
                            try:await current.send_bytes(packet)
                            except (ConnectionError,aiohttp.ClientError):break
                        await closed(current,(1003,1008,1009),label='invalid frame stream')
                    expect('DELETE','/v1/account/session',token=grants[1]);await asyncio.gather(closed(b,label='revoked party1'),closed(cb,label='revoked clan1'))
                    await ws('clan',1,status=401);grants[1]=login(1);current=await ws('clan',1)
                    checks.append('replacement, nonbinary frames, malformed version/size, replay, native burst budget and account revocation close the exact stream; fresh login reconnects')
                finally:
                    for wire in streams:await wire.close()
                if native_voice_probe:
                    for kind,participants in (('clan',(0,1)),('party',(1,2))):
                        group=(clan(0) if kind=='clan' else party(1))[kind]
                        config=dict(origin=base,clan='party' if kind=='party' else group['id'],party=kind=='party',idle_seconds=12)
                        for key,i in zip(('a','b'),participants):
                            identity=next(m['handle' if kind=='party' else 'id'] for m in group['members'] if m['name']=='Voice Pilot '+str(i))
                            config[key]=dict(token=grants[i],player=identity)
                        path=scratch/(kind+'-native-voice.json');path.write_text(json.dumps(config));path.chmod(0o600)
                        env=os.environ|dict(http_proxy='http://127.0.0.1:1',HTTP_PROXY='http://127.0.0.1:1',ALL_PROXY='http://127.0.0.1:1')
                        began=time.monotonic()
                        result=await asyncio.to_thread(subprocess.run,[str(native_voice_probe),str(path)],env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=40)
                        elapsed=time.monotonic()-began;assert result.returncode==0,(kind,result.returncode,result.stdout.decode(errors='replace')[-2000:])
                        native.append(dict(kind=kind,passed=True,seconds=elapsed,synthetic_pcm=True,opus=True,physical_audio=False,private_provider_fixture=True))
                    checks.append('existing native OnlineVoice clients exchange synthetic PCM through real Opus and Rust clan/party WebSockets, retain the idle heartbeat, and enforce opt-in capture, push-to-talk, participant/microphone mute and identity revocation')
                heartbeat=await ws('party',3,autoping=False);began=time.monotonic();row=await heartbeat.receive(timeout=12)
                assert row.type==WSMsgType.PING,row.type;timings['heartbeat_seconds']=time.monotonic()-began;assert timings['heartbeat_seconds']>=9.5
                await heartbeat.pong(row.data);row=await heartbeat.receive(timeout=12);assert row.type==WSMsgType.PING
                began=time.monotonic();await closed(heartbeat,(1011,),timeout=7);timings['missing_pong_seconds']=time.monotonic()-began
                assert timings['missing_pong_seconds']>=4.5
                checks.append('actual ten-second idle heartbeat accepts a pong and closes an unresponsive peer after the unshortened five-second transport deadline')
                shutdown=await ws('party',3)
                began=time.monotonic();await asyncio.gather(asyncio.to_thread(server.close),closed(shutdown,(1001,),label='SIGTERM'));timings['shutdown_seconds']=time.monotonic()-began
                assert server.process.returncode==0 and timings['shutdown_seconds']<3
                checks.append('real SIGTERM sends the Going Away close and retires native socket state before service exit')
        asyncio.run(media());server=None
        return dict(qualified=True,checks=checks,timings=timings,native_clients=native,upstream_identity_fixture='Private Steam SQL grants and canonical Discord-alias rows',real_provider=False,real_audio=False,native_played_world=False,original_parity=False)
    except BaseException as error:
        raise AssertionError(dict(error=repr(error),completed_checks=checks,timings=timings)) from error
    finally:
        if server:server.close()
