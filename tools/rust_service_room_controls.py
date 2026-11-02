"""Real Rust service room controls; Python supplies clients and private fixtures.

These are bounded modern transport/PCR controls, not played-world, original
Live arbitration, historical-profile or delivered-runtime acceptance.
"""
import asyncio
import hashlib
import json
import sqlite3
import os
import subprocess
import socket
import math
import time
import uuid


def qualify(binary,scratch,catalog,Server,native_probe=None):
    import aiohttp
    scratch.mkdir();state=scratch/'state';server=Server(binary,state,scratch,catalog)
    entries=json.loads(catalog.read_text())['playlists']
    maps=sorted({g['map'] for e in entries for g in e['games']})
    lab=next(e for e in entries if e['id']=='lab-test')
    tokens=['h2s_'+c*43 for c in 'RS']
    db=sqlite3.connect(state/'registry.sqlite3')
    with db:db.executemany('INSERT INTO steam_login_sessions VALUES(?,?,?,?)',[
        (hashlib.sha256(t.encode()).hexdigest(),'765611979602879'+str(i),'Room Fixture',int(time.time())+3600) for i,t in enumerate(tokens,70)])
    db.close();checks=[]
    async def run():
        base=f'http://{server.host}:{server.port}'
        async with aiohttp.ClientSession() as client:
            async def http(method,path,status,token=None,value=None):
                async with client.request(method,base+path,headers={'Authorization':'Bearer '+token} if token else {},json=value) as response:
                    result=await response.json()
                    assert response.status==status,(method,path,response.status,{'error':result.get('error'),'fields':sorted(result)})
                    assert response.headers.get('Cache-Control')=='no-store'
                    return result
            grants=[(await http('POST','/v1/matchmaking/session',201,token,
                dict(screen_name='Rust Room '+str(i),maps=maps,compatibility='halo2-native-queue-v16',language=0)))['token'] for i,token in enumerate(tokens)]
            sockets=[];tasks=[]
            async def echo(ws):
                async for message in ws:
                    if message.type!=aiohttp.WSMsgType.TEXT:break
                    data=json.loads(message.data)
                    if 'ping' in data:await ws.send_json({'pong':data['ping']})
                    elif 'probe' in data:await ws.send_json({'echo':data['probe']})
                    else:raise AssertionError('Unexpected native search control')
            try:
                for grant in grants:
                    ws=await client.ws_connect(base+'/v1/matchmaking/qos',headers={'Authorization':'Bearer '+grant},compress=0)
                    sockets.append(ws);tasks.append(asyncio.create_task(echo(ws)))
                    await http('POST','/v1/matchmaking/ticket',201,grant,dict(playlist_id=lab['id'],revision=lab['revision'],request_id=str(uuid.uuid4())))
                deadline=time.monotonic()+20;match=None
                while time.monotonic()<deadline:
                    for grant in grants:
                        row=await http('GET','/v1/matchmaking/ticket?progress=1',200,grant)
                        if row['match'] is not None:match=row;break
                    if match:break
                    await asyncio.sleep(.05)
                assert match is not None,'Real search sockets did not form the source-catalog Lab proposal'
                descriptors=[await http('GET','/v1/matchmaking/transport',200,grant) for grant in grants]
                assert descriptors[0]['id']==descriptors[1]['id']
                assert descriptors[0]['token']!=descriptors[1]['token']
                assert descriptors[0]['players']==descriptors[1]['players']
                assert all(d['transport']['kind']=='eos-p2p-v1' for d in descriptors)
                assert {d['slot'] for d in descriptors}=={0,1}
                assert len({p['identity'] for p in descriptors[0]['players']})==2
                for grant,previous in zip(grants,descriptors):
                    assert await http('GET','/v1/matchmaking/transport',200,grant)==previous
                await http('GET','/v1/matchmaking/transport',401)
                await http('GET','/v1/matchmaking/transport',401,tokens[0])
                assert (await http('GET','/health',200))['ready'] is False
                checks.append('real source-catalog v16 queue proposals produce stable private Rust room credentials, opaque roster identities and EOS descriptors without a Python coordinator')
            finally:
                for ws in sockets:await ws.close()
                await asyncio.gather(*tasks)
    try:asyncio.run(run())
    finally:server.close()
    result=dict(checks=checks,relay=relay(binary,scratch/'relay',catalog,Server),
        scope='Actual Rust HTTP/SQLite/room admission on the source catalog and declared relay fixture; private provider fixtures. Played-world/EOS provider and original/native/delivered acceptance remain separate.')
    result['direct']=direct(binary,scratch/'direct',scratch/'relay/catalog.json',Server)
    result['content_failure']=content_failure(binary,scratch/'content-failure',scratch/'relay/catalog.json',Server)
    if native_probe:result['native']=native(binary,scratch/'native',scratch/'relay/catalog.json',Server,native_probe)
    return result


def relay(binary,scratch,catalog,Server):
    import aiohttp
    scratch.mkdir();entries=json.loads(catalog.read_text())['playlists']
    # Retained legacy typed-IO seam, based on test_matchmaking_http.catalog.
    # Full hopper settings retain their production v16 compatibility gate.
    # This declared FFA fixture adds a guest and personal hidden XP; it neither
    # changes the source catalog nor supplies original-service configuration.
    lab=dict(id='rust-room-fixture',revision='rust-room-fixture-v1',name='Rust room fixture',
        description='Declared Rust relay fixture.',ranked=False,quickmatch=False,team_size=0,
        minimum_players=3,maximum_players=16,maximum_party=2,skill=dict(kind='custom',radius=0),
        stages=[dict(after_seconds=0,rank_numerator=1,rank_denominator=1,minimum_connection=50)],
        required_compatibility='halo2-native-queue-v10',games=[dict(id='slayer',map='lockout',variant='Slayer',weight=1)],
        rank_configuration=next(p['rank_configuration'] for p in entries if p['id']=='team-slayer'),
        provenance='Declared Rust room legacy relay/hidden-XP integration fixture. No original profile, complete hopper options or played-match claim.')
    fixture=scratch/'catalog.json';fixture.write_text(json.dumps(dict(version=1,playlists=[lab])))
    state=scratch/'state';server=Server(binary,state,scratch,fixture)
    tokens=['h2s_'+c*43 for c in 'TU'];checks=[];observed={}
    db=sqlite3.connect(state/'registry.sqlite3')
    with db:db.executemany('INSERT INTO steam_login_sessions VALUES(?,?,?,?)',[
        (hashlib.sha256(t.encode()).hexdigest(),'765611979602879'+str(i),'Relay Fixture',int(time.time())+3600) for i,t in enumerate(tokens,72)])
    db.close()
    async def run():
        base=f'http://{server.host}:{server.port}';qos=[];room_sockets=[];tasks=[]
        async with aiohttp.ClientSession() as client:
            async def http(method,path,status,token=None,value=None):
                async with client.request(method,base+path,headers={'Authorization':'Bearer '+token} if token else {},json=value) as response:
                    result=await response.json()
                    assert response.status==status,(method,path,response.status,{'error':result.get('error'),'fields':sorted(result)})
                    return result
            def sql(statement,params=()):
                with sqlite3.connect(state/'registry.sqlite3') as connection:return connection.execute(statement,params).fetchall()
            async def denial(path,token,status):
                try:await client.ws_connect(base+path,headers={'Authorization':'Bearer '+token},compress=0)
                except aiohttp.WSServerHandshakeError as error:assert error.status==status,(path,error.status,status)
                else:raise AssertionError('Unexpected room upgrade')
            async def reader(ws,queue=None):
                async for message in ws:
                    if message.type==aiohttp.WSMsgType.TEXT:
                        data=json.loads(message.data)
                        if 'ping' in data:await ws.send_json({'pong':data['ping']})
                        elif 'probe' in data:await ws.send_json({'echo':data['probe']})
                        elif queue is not None:await queue.put(data)
                        else:raise AssertionError('Unexpected search frame')
                    elif message.type==aiohttp.WSMsgType.BINARY and queue is not None:await queue.put(message.data)
                if queue is not None:await queue.put(None)
            async def until(queue,predicate,timeout=5):
                deadline=time.monotonic()+timeout
                while time.monotonic()<deadline:
                    item=await asyncio.wait_for(queue.get(),deadline-time.monotonic())
                    assert item is not None,'Actual room stream closed before the expected native publication'
                    if predicate(item):return item
                raise AssertionError('Native room publication deadline')
            body=lambda i:dict(screen_name='Relay Pilot '+str(i),maps=['lockout'],compatibility='halo2-native-queue-v10',language=0,
                local_players=[dict(screen_name='Relay Guest')] if i==0 else [])
            grants=[(await http('POST','/v1/matchmaking/session',201,t,body(i)))['token'] for i,t in enumerate(tokens)]
            request=lambda:dict(playlist_id=lab['id'],revision=lab['revision'],request_id=str(uuid.uuid4()))
            try:
                for grant in grants:
                    ws=await client.ws_connect(base+'/v1/matchmaking/qos',headers={'Authorization':'Bearer '+grant},compress=0)
                    qos.append(ws);tasks.append(asyncio.create_task(reader(ws)))
                    await http('POST','/v1/matchmaking/ticket',201,grant,request())
                deadline=time.monotonic()+20;match=None
                while time.monotonic()<deadline:
                    for grant in grants:
                        view=await http('GET','/v1/matchmaking/ticket?progress=1',200,grant)
                        if view['match'] is not None:match=view;break
                    if match:break
                    await asyncio.sleep(.05)
                assert match is not None and len(match['match']['players'])==3
                with sqlite3.connect(state/'registry.sqlite3') as connection:
                    connection.executescript("CREATE TABLE room_failure_parent(id TEXT PRIMARY KEY);CREATE TABLE room_commit_failure(id TEXT REFERENCES room_failure_parent(id) DEFERRABLE INITIALLY DEFERRED);CREATE TRIGGER room_fail_admission AFTER UPDATE ON matchmaking_ledger_lock BEGIN INSERT INTO room_commit_failure VALUES('missing'); END;")
                await http('GET','/v1/matchmaking/transport',500,grants[0])
                assert sql('SELECT count(*) FROM room_commit_failure')[0][0]==0
                sql('DROP TRIGGER room_fail_admission')
                descriptors=[await http('GET','/v1/matchmaking/transport',200,grant) for grant in grants]
                assert descriptors[0]['id']==descriptors[1]['id']
                assert len(descriptors[0]['players'])==3 and len(descriptors[0]['local_slots'])==2
                assert sum(p['guest'] for p in descriptors[0]['players'])==1
                assert all('transport' not in d for d in descriptors)
                assert sql('SELECT count(*) FROM playlist_skill')[0][0]==2
                assert sql('SELECT count(*) FROM match_results')[0][0]==0
                checks.append('real failed room-admission COMMIT rolls back all provisional result/setup credentials and retries the same admitted roster once; guests stay transient')
                path='/v1/matches/'+descriptors[0]['id']
                await denial(path+'/relay','wrong',401)
                await denial(path+'/signal',descriptors[0]['token'],409)
                queues=[]
                for descriptor in descriptors:
                    ws=await client.ws_connect(base+path+'/relay?completion=1',headers={'Authorization':'Bearer '+descriptor['token']},compress=0)
                    room_sockets.append(ws);queue=asyncio.Queue();queues.append(queue);tasks.append(asyncio.create_task(reader(ws,queue)))
                loading=[]
                for queue in queues:loading.append(await until(queue,lambda r:isinstance(r,dict) and r.get('phase')=='loading'))
                host=loading[0]['host'];assert all(r['host']==host for r in loading)
                counts=await http('GET','/v1/online/status',200)
                assert counts['joining']=={'users':2,'guests':1} and counts['online']==counts['joining']
                await denial(path+'/relay',descriptors[0]['token'],409)
                first=await http('GET','/v1/matchmaking/ticket?start=1',200,grants[0]);remaining=first['start_countdown_ms']
                assert 0<remaining<=10000
                second=await http('GET','/v1/matchmaking/ticket?start=1',200,grants[1])
                assert abs(second['start_countdown_ms']-remaining)<250
                started=time.monotonic()
                for ws in room_sockets:await ws.send_json({'loaded':'0123456789abcdef:000001a'})
                for queue in queues:await until(queue,lambda r:isinstance(r,dict) and r.get('phase')=='running',13)
                elapsed=time.monotonic()-started;assert elapsed>=remaining/1000-.25
                observed.update(start_countdown_ms=remaining,real_start_elapsed_seconds=elapsed,physical_connections=2,logical_seats=3)
                assert (await http('GET','/v1/online/status',200))['playing']=={'users':2,'guests':1}
                await http('POST','/v1/matchmaking/ticket',409,grants[0],request())
                checks.append('actual room challenges select a host, publish a shared unshortened ten-second loading countdown and bind live room observations into queue/session admission')
                h=next(i for i,d in enumerate(descriptors) if d['slot']==host);c=1-h
                packet=b'H2N2\x1a\x00\x01'+b'x'*20
                await room_sockets[c].send_bytes(bytes([host])+packet)
                assert await until(queues[h],lambda r:isinstance(r,bytes))==bytes([descriptors[c]['slot']])+packet
                snapshot=b'H2N2\x1a\x00\x03'+b'y'*1393
                await room_sockets[h].send_bytes(bytes([descriptors[c]['slot']])+snapshot)
                assert await until(queues[c],lambda r:isinstance(r,bytes))==bytes([host])+snapshot
                await http('DELETE','/v1/matchmaking/session',200,grants[c])
                assert (await http('GET','/v1/online/status',200))['playing']=={'users':2,'guests':1}
                local=body(h);local['local_players'].append(dict(screen_name='Foreign Active Seat',identity_token=tokens[c]))
                await http('POST','/v1/matchmaking/session',409,tokens[h],local)
                await room_sockets[c].send_bytes(bytes([host])+packet)
                assert await until(queues[h],lambda r:isinstance(r,bytes))==bytes([descriptors[c]['slot']])+packet
                checks.append('real binary relay enforces native sender/target roles, forwards the complete1400-byte snapshot and preserves a transferred room bearer after frontend logout')
                raw=bytearray(0xdc78);raw[:2]=raw[0x10:0x12]=b'\x01\x01';raw[0x64]=2
                for slot,row in enumerate(descriptors[0]['players']):
                    at=0x370+slot*0x94;raw[at]=1;raw[at+2:at+10]=bytes.fromhex(row['identity'])
                    name=row['name'].encode('utf-16-le');raw[at+12:at+12+len(name)]=name
                    raw[at+0x88]=255;raw[at+0x90]=0 if slot==descriptors[0]['slot'] else 1
                digest=hashlib.sha256(raw).hexdigest()
                await room_sockets[h].send_json({'postgame':raw.hex()})
                for queue in queues:assert (await until(queue,lambda r:isinstance(r,dict) and 'postgame' in r))['postgame']==raw.hex()
                await room_sockets[h].send_json({'confirm_result':digest});await asyncio.sleep(.1)
                assert sql('SELECT count(*) FROM match_results')[0][0]==0
                sql("CREATE TRIGGER room_fail_result AFTER INSERT ON match_results BEGIN INSERT INTO room_commit_failure VALUES('missing'); END")
                await room_sockets[c].send_json({'confirm_result':digest})
                await asyncio.sleep(.25)
                assert sql('SELECT count(*) FROM match_results')[0][0]==0
                assert sql('SELECT count(*) FROM match_result_skill')[0][0]==0
                for queue in queues:
                    while not queue.empty():
                        pending=queue.get_nowait()
                        assert pending is not None and not (isinstance(pending,dict) and 'completed' in pending),'Failed COMMIT published completion'
                sql('DROP TRIGGER room_fail_result')
                for queue in queues:
                    completed=await until(queue,lambda r:isinstance(r,dict) and 'completed' in r)
                    assert completed==dict(completed=digest,recorded=True)
                assert sql('SELECT count(*) FROM match_results')[0][0]==1
                assert sql('SELECT count(*) FROM match_result_players')[0][0]==2
                assert sql('SELECT count(*) FROM match_result_skill')[0][0]==2
                assert sql('SELECT count(*) FROM carnage_reports')[0][0]==1
                before=sql('SELECT player,raw_rank,xp FROM playlist_skill ORDER BY player')
                assert sum(r[2] for r in before)>0
                for ws in room_sockets:await ws.send_json({'confirm_result':digest})
                await asyncio.sleep(.1)
                assert sql('SELECT count(*) FROM match_results')[0][0]==1
                assert sql('SELECT player,raw_rank,xp FROM playlist_skill ORDER BY player')==before
                metadata=json.loads(sql('SELECT metadata FROM match_result_admission')[0][0])
                assert metadata['admission']['proposal']['game']['variant']=='Slayer'
                assert metadata['admission']['compatibility']=='halo2-native-queue-v10'
                observed['persistent_xp_total']=sum(r[2] for r in before)
                checks.append('actual Rust socket report/unanimous confirmation retains review on failed COMMIT, publishes nothing early and retries the write once; admitted legacy variant, hidden XP, public PCR and guest exclusion persist exactly once')
                counts=await http('GET','/v1/online/status',200)
                assert counts['postgame']=={'users':2,'guests':1} and counts['playing']=={'users':0,'guests':0}
                checks.append('real anonymous population follows loading/running/postgame room owners and the transient guest, retaining active playing seats after frontend logout')
                renewed=(await http('POST','/v1/matchmaking/session',201,tokens[0],body(0)))['token']
                assert (await http('POST','/v1/matchmaking/ticket',201,renewed,request()))['state']=='searching'
                checks.append('postgame room ownership allows an independently renewed frontend session and ordinary requeue without duplicating the committed result')
            finally:
                for ws in room_sockets+qos:await ws.close()
                await asyncio.gather(*tasks)
    try:asyncio.run(run())
    finally:server.close()
    return dict(checks=checks,observed=observed,fixture=dict(compatibility='halo2-native-queue-v10',catalog_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),
        declared=True,original_profile=False,real_gameplay=False),scope='Real Rust HTTP/SQLite/WebSocket relay and PCR fixture, real ten-second start; synthetic datagrams/results and private provider grants. No played-game, original or delivered acceptance.')


def native(binary,scratch,fixture,Server,probe):
    scratch.mkdir();catalog=json.loads(fixture.read_text());row=catalog['playlists'][0]
    row.update(id='rust-native-room',revision='native-room-v1',minimum_players=2,maximum_party=1)
    path=scratch/'catalog.json';path.write_text(json.dumps(catalog));state=scratch/'state'
    server=Server(binary,state,scratch,path)
    try:
        tokens=['h2s_'+c*43 for c in 'Za']
        with sqlite3.connect(state/'registry.sqlite3') as db:
            db.executemany('INSERT INTO steam_login_sessions VALUES(?,?,?,?)',[
                (hashlib.sha256(t.encode()).hexdigest(),'765611979602879'+str(i),'Native Room Fixture',int(time.time())+3600) for i,t in enumerate(tokens,90)])
        def http(method,path,status,**kwargs):
            actual,value=server.request(method,path,**kwargs)
            assert actual==status,(path,actual,{'error':value.get('error'),'fields':sorted(value)})
            return value
        grants=[http('POST','/v1/matchmaking/session',201,token=token,body=dict(screen_name='Native Room '+str(i),maps=['lockout'],
            compatibility='halo2-native-queue-v10',language=0))['token'] for i,token in enumerate(tokens)]
        for grant in grants:http('POST','/v1/matchmaking/ticket',201,token=grant,body=dict(playlist_id=row['id'],revision=row['revision'],request_id=str(uuid.uuid4())))
        descriptors=[http('GET','/v1/matchmaking/transport',200,token=grant) for grant in grants]
        run=subprocess.run([str(probe)],env=dict(os.environ,H2_TEST_ROOM_URL=f'http://{server.host}:{server.port}',H2_TEST_ROOM_DESCRIPTORS=json.dumps(descriptors)),
            stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=45)
        text=run.stdout.decode(errors='replace')
        assert run.returncode==0,('Native room probe failed',run.returncode,text[-2000:])
        lines=[line for line in text.splitlines() if line.startswith('PASS native Rust room clients ')]
        assert len(lines)==1
        observed=json.loads(lines[0].removeprefix('PASS native Rust room clients '))
        assert observed['players']==2 and observed['snapshot_bytes']==1400 and observed['result_recorded'] and observed['real_start_timer']
        with sqlite3.connect(state/'registry.sqlite3') as db:
            assert db.execute('SELECT count(*) FROM match_results').fetchone()[0]==1
            assert db.execute('SELECT count(*) FROM match_result_skill').fetchone()[0]==2
        return dict(passed=True,observed=observed,scope='Actual native OnlineMatchSession clients, real Rust loading/start, datagram relay and unanimous PCR/XP commit; declared legacy profile, synthetic datagrams/PCR/fingerprint and fixture provider grants. No native world/rendered game or delivered acceptance.')
    finally:server.close()


def direct(binary,scratch,fixture,Server):
    import aiohttp
    scratch.mkdir();catalog=json.loads(fixture.read_text());row=catalog['playlists'][0]
    row.update(id='rust-direct-room',revision='direct-room-v1',minimum_players=2,maximum_party=1)
    path=scratch/'catalog.json';path.write_text(json.dumps(catalog));state=scratch/'state';server=Server(binary,state,scratch,path)
    tokens=['h2s_'+c*43 for c in 'bc'];checks=[];observed={}
    with sqlite3.connect(state/'registry.sqlite3') as db:
        db.executemany('INSERT INTO steam_login_sessions VALUES(?,?,?,?)',[
            (hashlib.sha256(t.encode()).hexdigest(),'765611979602880'+str(i),'Direct Fixture',int(time.time())+3600) for i,t in enumerate(tokens,10)])
    async def run():
        base=f'http://{server.host}:{server.port}';connections=[];tasks=[]
        async with aiohttp.ClientSession() as client:
            async def http(method,path,status,token=None,value=None):
                async with client.request(method,base+path,headers={'Authorization':'Bearer '+token} if token else {},json=value) as response:
                    result=await response.json()
                    assert response.status==status,(path,response.status,{'error':result.get('error'),'fields':sorted(result)})
                    return result
            async def reader(ws,queue):
                async for message in ws:
                    assert message.type!=aiohttp.WSMsgType.BINARY,'Direct coordinator relayed media'
                    if message.type==aiohttp.WSMsgType.TEXT:
                        data=json.loads(message.data)
                        if 'ping' in data:await ws.send_json({'pong':data['ping']})
                        else:await queue.put(data)
                await queue.put(None)
            async def connect(descriptor):
                ws=await client.ws_connect(base+'/v1/matches/'+descriptor['id']+'/signal?completion=1&native_handoff=1&departure_game_over=1',
                    headers={'Authorization':'Bearer '+descriptor['token']},compress=0)
                queue=asyncio.Queue();connections.append(ws);tasks.append(asyncio.create_task(reader(ws,queue)))
                return ws,queue
            async def until(queue,key,value=None,timeout=5):
                deadline=time.monotonic()+timeout
                while time.monotonic()<deadline:
                    item=await asyncio.wait_for(queue.get(),deadline-time.monotonic())
                    assert item is not None,'Direct room stream closed before its native publication'
                    if key in item and (value is None or item[key]==value):return item
                raise AssertionError('Direct native publication deadline')
            grants=[(await http('POST','/v1/matchmaking/session',201,token,dict(screen_name='Direct Pilot '+str(i),maps=['lockout'],
                compatibility='halo2-native-queue-v12',language=0)))['token'] for i,token in enumerate(tokens)]
            for grant in grants:await http('POST','/v1/matchmaking/ticket',201,grant,dict(playlist_id=row['id'],revision=row['revision'],request_id=str(uuid.uuid4())))
            descriptors=[await http('GET','/v1/matchmaking/transport',200,grant) for grant in grants]
            assert all(d['transport']==dict(kind='direct-udp-v1',stun_servers=[]) for d in descriptors)
            low,high=sorted(descriptors,key=lambda d:d['slot'])
            udp=[socket.socket(socket.AF_INET,socket.SOCK_DGRAM) for _ in range(2)]
            for wire in udp:wire.bind(('127.0.0.1',0));wire.setblocking(False)
            try:
                # Actual localhost UDP exchanges supply these test clients'
                # reported RTTs. They remain client claims, not XNet readings
                # or a coordinator attestation of direct gameplay.
                loop=asyncio.get_running_loop();samples=[[],[]]
                for i in range(2):
                    for _ in range(3):
                        challenge=os.urandom(16);started=time.monotonic_ns()
                        await loop.sock_sendto(udp[i],challenge,udp[1-i].getsockname())
                        data,address=await asyncio.wait_for(loop.sock_recvfrom(udp[1-i],64),2)
                        assert data==challenge and address==udp[i].getsockname()
                        await loop.sock_sendto(udp[1-i],data,address)
                        answer,address=await asyncio.wait_for(loop.sock_recvfrom(udp[i],64),2)
                        assert answer==challenge and address==udp[1-i].getsockname()
                        samples[i].append(math.ceil((time.monotonic_ns()-started)/1_000_000))
                a,qa=await connect(low);await until(qa,'phase','connecting')
                port=udp[0].getsockname()[1]
                await a.send_json({'peer_signal':dict(peer=high['slot'],type='candidate',payload=f'candidate:1 1 UDP 1 127.0.0.1 {port} typ host',mid='0')})
                b,qb=await connect(high)
                signal=(await until(qb,'peer_signal'))['peer_signal']
                assert signal['peer']==low['slot'] and str(port) in signal['payload']
                for i,(ws,other) in enumerate(((a,high),(b,low))):
                    await ws.send_json({'peer_ready':[dict(peer=other['slot'],rtt_ms=sorted(samples[i])[1])]})
                first=await until(qa,'phase','loading');second=await until(qb,'phase','loading')
                assert first['host']==second['host']
                started=time.monotonic()
                for ws in (a,b):await ws.send_json({'loaded':'0123456789abcdef:000001a'})
                await until(qa,'phase','running',13);await until(qb,'phase','running',13)
                assert time.monotonic()-started>=9
                checks.append('actual signal route drains a buffered candidate, validates physical peer reports and reaches running through the real ten-second gate; test RTT claims come from real local UDP exchanges')
                replacement,replaced_queue=await connect(low)
                await until(replaced_queue,'phase','running')
                deadline=time.monotonic()+3
                while not a.closed and time.monotonic()<deadline:await asyncio.sleep(.02)
                assert a.closed and a.close_code==1001
                i_low=next(i for i,d in enumerate(descriptors) if d['slot']==low['slot'])
                await http('DELETE','/v1/matchmaking/session',200,grants[i_low])
                await replacement.send_json({'peer_signal':dict(peer=high['slot'],type='candidate',payload='candidate:2 1 UDP 1 127.0.0.1 '+str(port)+' typ host',mid='0')})
                assert (await until(qb,'peer_signal'))['peer_signal']['peer']==low['slot']
                assert not replacement.closed and not b.closed
                checks.append('live direct socket replacement closes the old generation with its native reason; its late detach cannot remove the replacement, and frontend logout leaves the transferred bearer valid')
                await replacement.send_json({'leave':True})
                notice=await until(qb,'departed_players')
                assert notice==dict(departed_players=1<<low['slot']) and not b.closed
                with sqlite3.connect(state/'registry.sqlite3') as db:assert db.execute('SELECT count(*) FROM match_results').fetchone()[0]==0
                checks.append('explicit direct departure publishes the frozen departed-player mask to the survivor without inventing a result or closing its room')
                observed.update(physical_connections=2,udp_probe_exchanges=6,client_reported_rtt_ms=[sorted(s)[1] for s in samples],replaced_close_code=a.close_code)
            finally:
                for wire in udp:wire.close()
                for ws in connections:await ws.close()
                await asyncio.gather(*tasks)
    try:asyncio.run(run())
    finally:server.close()
    return dict(checks=checks,observed=observed,scope='Actual Rust direct signaling/clock/generation/departure IO on a declared v12 legacy fixture, private grants and real local UDP client RTT probes. No native peer/gameplay/world promotion, original XNet or delivered acceptance.')


def content_failure(binary,scratch,fixture,Server):
    import aiohttp
    scratch.mkdir();catalog=json.loads(fixture.read_text());row=catalog['playlists'][0]
    row.update(id='rust-content-room',revision='content-room-v1',minimum_players=2,maximum_party=1)
    path=scratch/'catalog.json';path.write_text(json.dumps(catalog));state=scratch/'state';server=Server(binary,state,scratch,path)
    tokens=['h2s_'+c*43 for c in 'de'];checks=[]
    with sqlite3.connect(state/'registry.sqlite3') as db:
        db.executemany('INSERT INTO steam_login_sessions VALUES(?,?,?,?)',[
            (hashlib.sha256(t.encode()).hexdigest(),'765611979602880'+str(i),'Content Fixture',int(time.time())+3600) for i,t in enumerate(tokens,30)])
    async def run():
        base=f'http://{server.host}:{server.port}';sockets=[];tasks=[]
        async with aiohttp.ClientSession() as client:
            async def http(method,path,status,token=None,value=None):
                async with client.request(method,base+path,headers={'Authorization':'Bearer '+token} if token else {},json=value) as response:
                    data=await response.json()
                    assert response.status==status,(path,response.status,{'error':data.get('error'),'fields':sorted(data)})
                    return data
            async def reader(ws,queue):
                async for message in ws:
                    if message.type==aiohttp.WSMsgType.TEXT:
                        data=json.loads(message.data)
                        if 'ping' in data:await ws.send_json({'pong':data['ping']})
                        else:await queue.put(data)
                await queue.put(None)
            async def loading(queue):
                async with asyncio.timeout(5):
                    while True:
                        value=await queue.get();assert value is not None
                        if value.get('phase')=='loading':return
            grants=[(await http('POST','/v1/matchmaking/session',201,token,dict(screen_name='Content Pilot '+str(i),maps=['lockout'],
                compatibility='halo2-native-queue-v10',language=0)))['token'] for i,token in enumerate(tokens)]
            request=lambda:dict(playlist_id=row['id'],revision=row['revision'],request_id=str(uuid.uuid4()))
            for grant in grants:await http('POST','/v1/matchmaking/ticket',201,grant,request())
            descriptors=[await http('GET','/v1/matchmaking/transport',200,grant) for grant in grants]
            try:
                queues=[]
                for descriptor in descriptors:
                    ws=await client.ws_connect(base+'/v1/matches/'+descriptor['id']+'/relay',headers={'Authorization':'Bearer '+descriptor['token']},compress=0)
                    sockets.append(ws);queue=asyncio.Queue();queues.append(queue);tasks.append(asyncio.create_task(reader(ws,queue)))
                for queue in queues:await loading(queue)
                body=dict(match_id=descriptors[0]['id'],kind='missing_content')
                await http('POST','/v1/matchmaking/failure',401,None,body)
                await http('POST','/v1/matchmaking/failure',403,grants[0],body|{'match_id':str(uuid.uuid4())})
                assert await http('POST','/v1/matchmaking/failure',200,grants[0],body)==dict(accepted=True)
                for queue in queues:
                    async with asyncio.timeout(3):
                        while await queue.get() is not None:pass
                for grant,kind in zip(grants,('missing_content','search_failed')):
                    value=await http('GET','/v1/matchmaking/ticket?failure=1&start=1',200,grant)
                    assert value['failure']==kind and value['start_countdown_ms'] is None
                # The native retained failure origin permits this same late
                # content receipt after room teardown; a new search clears it.
                assert await http('POST','/v1/matchmaking/failure',200,grants[0],body)==dict(accepted=True)
                await http('POST','/v1/matchmaking/ticket',201,grants[0],request())
                assert (await http('GET','/v1/matchmaking/ticket?failure=1',200,grants[0]))['failure'] is None
                await http('POST','/v1/matchmaking/failure',409,grants[0],body)
                checks.append('actual loading-content rejection requires the current grant/room, closes the real streams, preserves actor/other-party failure codes after teardown and fences stale receipts after requeue')
            finally:
                for ws in sockets:await ws.close()
                await asyncio.gather(*tasks)
    try:asyncio.run(run())
    finally:server.close()
    return dict(checks=checks,scope='Real Rust HTTP/room-close/failure IO on a declared legacy fixture; no rendered map-load or played-world acceptance.')
