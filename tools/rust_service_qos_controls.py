"""Real WebSocket controls for run_test.py-owned Rust service qualification.

Python is a client/oracle here. The real Rust process owns HTTP, WebSockets,
matching, sessions and storage; grants are seeded only into its private test DB.
"""
import asyncio
import base64
import hashlib
import json
import os
from pathlib import Path
import selectors
import signal
import socket
import sqlite3
import subprocess
import time
import uuid


def qualify(binary,scratch,catalog,Server,native_probe=None):
    import aiohttp
    scratch.mkdir();state=scratch/'state';server=Server(binary,state,scratch,catalog)
    entries=json.loads(catalog.read_text())['playlists']
    maps=sorted({g['map'] for e in entries for g in e['games']})
    lab=next(e for e in entries if e['id']=='lab-test')
    tokens=['h2s_'+c*43 for c in 'HI'];subjects=['76561197960287943','76561197960287944']
    db=sqlite3.connect(state/'registry.sqlite3')
    with db:db.executemany('INSERT INTO steam_login_sessions VALUES(?,?,?,?)',[
        (hashlib.sha256(t.encode()).hexdigest(),s,'Steam Fixture',int(time.time())+3600) for t,s in zip(tokens,subjects)])
    db.close();checks=[];counts={'pings':0,'probes':0}
    body=dict(screen_name='QoS Pilot',maps=maps,compatibility='halo2-native-queue-v16',language=0,
              local_players=[dict(screen_name='QoS Guest')])
    queue_tokens=[]
    def raw_unmasked(token):
        with socket.create_connection((server.host,server.port),timeout=3) as wire:
            key=base64.b64encode(bytes(range(16))).decode()
            wire.sendall((f'GET /v1/matchmaking/qos HTTP/1.1\r\nHost: localhost\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n'
                f'Sec-WebSocket-Version: 13\r\nSec-WebSocket-Key: {key}\r\nAuthorization: Bearer {token}\r\n\r\n').encode())
            reply=b''
            while b'\r\n\r\n' not in reply:
                chunk=wire.recv(4096);assert chunk,'Search upgrade ended early'
                reply+=chunk;assert len(reply)<16384
            assert b' 101 ' in reply.split(b'\r\n')[0]
            data=bytearray(reply.split(b'\r\n\r\n',1)[1])
            def frame():
                def fill(size):
                    while len(data)<size:
                        try:chunk=wire.recv(4096)
                        except ConnectionResetError:return False
                        if not chunk:return False
                        data.extend(chunk);assert len(data)<=4096
                    return True
                if not fill(2):return None
                assert data[0]&0x80 and not data[1]&0x80
                size=data[1]&127;assert size<126
                if not fill(size+2):return None
                opcode=data[0]&15;payload=bytes(data[2:size+2]);del data[:size+2]
                return opcode,payload
            initial=frame();assert initial and initial[0]==1
            heartbeat=json.loads(initial[1]);assert set(heartbeat)=={'ping'}
            # This exact reply is valid application data. Only its missing
            # RFC6455 mask differs from an accepted heartbeat response.
            payload=json.dumps({'pong':heartbeat['ping']},separators=(',',':')).encode()
            assert len(payload)<126
            wire.sendall(bytes((0x81,len(payload)))+payload)
            try:rejected=frame()
            except TimeoutError:raise AssertionError('Valid unmasked heartbeat was not rejected') from None
            assert rejected is None or rejected[0]==8,'Valid unmasked heartbeat was accepted'
    async def run():
        base=f'http://{server.host}:{server.port}'
        async with aiohttp.ClientSession() as client:
            async def http(method,path,status,token=None,value=None):
                async with client.request(method,base+path,headers={'Authorization':'Bearer '+token} if token else {},json=value) as response:
                    result=await response.json()
                    assert response.status==status,(method,path,response.status,{'error':result.get('error'),'fields':sorted(result)})
                    assert response.headers.get('Cache-Control')=='no-store'
                    assert response.headers.get('Connection')=='close'
                    return result
            async def connect(token,status=101):
                try:
                    ws=await client.ws_connect(base+'/v1/matchmaking/qos',headers={'Authorization':'Bearer '+token},compress=0,max_msg_size=256)
                except aiohttp.WSServerHandshakeError as error:
                    assert status!=101 and error.status==status,('Search handshake',error.status,status,
                        {'connection':error.headers.get('Connection'),'upgrade':error.headers.get('Upgrade'),'message':error.message})
                    return None
                assert status==101,('Unexpected search upgrade',status)
                assert ws._response.headers.get('Sec-WebSocket-Extensions') is None
                assert ws._response.headers.get('Cache-Control')=='no-store'
                assert ws._response.headers.get('Connection').lower()=='upgrade'
                return ws
            async def attached(ws):
                initial=await ws.receive_json(timeout=3)
                assert set(initial)=={'ping'}
                counts['pings']+=1
                await ws.send_json({'pong':initial['ping']})
            async def closed(ws,timeout=3):
                deadline=time.monotonic()+timeout
                while time.monotonic()<deadline:
                    reply=await ws.receive(timeout=max(.1,deadline-time.monotonic()))
                    if reply.type in (aiohttp.WSMsgType.CLOSE,aiohttp.WSMsgType.CLOSED,aiohttp.WSMsgType.CLOSING,aiohttp.WSMsgType.ERROR):return
                raise AssertionError('Search socket was not closed')
            async def echo(ws):
                async for message in ws:
                    if message.type!=aiohttp.WSMsgType.TEXT:break
                    payload=json.loads(message.data)
                    assert len(payload)==1 and set(payload)<=set(('ping','probe'))
                    if 'ping' in payload:
                        counts['pings']+=1;await ws.send_json({'pong':payload['ping']})
                    else:
                        counts['probes']+=1;await ws.send_json({'echo':payload['probe']})
            for token,row in zip(tokens,(body,body|{'screen_name':'QoS Peer','local_players':[]})):
                queue_tokens.append((await http('POST','/v1/matchmaking/session',201,token,row))['token'])
            a,b=queue_tokens
            await connect('invalid',401)
            await connect(tokens[0],401)
            ws=await connect(a)
            await attached(ws)
            await connect(a,409)
            await ws.close()
            checks.append('real search upgrade requires a matchmaking grant and rejects duplicate live physical connections')
            for data,kind in (('{"pong":"x","pong":"y"}','text'),('{"rank":50}','text'),('x'*257,'text'),(b'\x01','binary')):
                ws=await connect(a)
                if kind=='text':await ws.send_str(data)
                else:await ws.send_bytes(data)
                await closed(ws);await ws.close()
            await asyncio.to_thread(raw_unmasked,a)
            checks.append('actual WebSocket parser/native controls reject duplicate JSON, invented controls, oversized messages, binary and unmasked frames')
            old=await connect(a)
            await attached(old)
            a=(await http('POST','/v1/matchmaking/session',201,tokens[0],body))['token']
            queue_tokens[0]=a
            current=await connect(a)
            await attached(current)
            await closed(old);await old.close()
            await connect(a,409)
            await connect(queue_tokens[1]+'x',401)
            checks.append('real session replacement closes the old socket and its late detach cannot retire the replacement')
            reader=asyncio.create_task(echo(current))
            silent=await connect(b)
            first=await silent.receive_json(timeout=3)
            assert set(first)=={'ping'}
            started=time.monotonic()
            await closed(silent,18)
            assert time.monotonic()-started>=14
            await silent.close()
            # The old HTTP listener has a30s connection deadline. A genuine
            # upgrade must survive it while native heartbeats are answered.
            await asyncio.sleep(max(0,31-(time.monotonic()-started)))
            assert not current.closed and not reader.done() and counts['pings']>=6
            checks.append('native application heartbeat expires a silent socket while an answering upgraded connection survives beyond the HTTP30s deadline')
            peer=await connect(b);peer_reader=asyncio.create_task(echo(peer))
            request=lambda:dict(playlist_id=lab['id'],revision=lab['revision'],request_id=str(uuid.uuid4()))
            await http('POST','/v1/matchmaking/ticket',201,a,request())
            await http('POST','/v1/matchmaking/ticket',201,b,request())
            match=None;deadline=time.monotonic()+20
            while time.monotonic()<deadline:
                for who in (a,b):
                    view=await http('GET','/v1/matchmaking/ticket?progress=1&ranks=1&start=1&failure=1&appearance=1',200,who)
                    if view['match'] is not None:match=view;break
                if match:break
                await asyncio.sleep(.05)
            assert match is not None,'Actual socket QoS did not produce a Lab gathering proposal'
            assert counts['probes']>=2
            assert len(match['match']['players'])==3
            assert {row['name'] for row in match['match']['players']}=={'QoS Pilot','QoS Guest','QoS Peer'}
            public=json.dumps(match)
            for private in (*tokens,*subjects,*queue_tokens):assert private not in public
            checks.append('actual physical socket echoes produce native relay measurements and a real authored Lab proposal with two owners and a transient guest')
            for who in (a,b):await http('DELETE','/v1/matchmaking/ticket?cancel=1',200,who)
            await current.close();await peer.close();await asyncio.gather(reader,peer_reader)
            await http('DELETE','/v1/matchmaking/session',200,a)
            await connect(a,401)
            assert (await http('GET','/health',200))['ready'] is False
            checks.append('socket-backed search cancellation/revocation retains grant fencing and never advertises full room/gameplay readiness')
    try:
        asyncio.run(run())
        old=queue_tokens[1];server.close();server=Server(binary,state,scratch,catalog)
        async def restart():
            async with aiohttp.ClientSession() as client:
                try:await client.ws_connect(f'http://{server.host}:{server.port}/v1/matchmaking/qos',headers={'Authorization':'Bearer '+old})
                except aiohttp.WSServerHandshakeError as error:assert error.status==401
                else:raise AssertionError('Search grant survived process restart')
        asyncio.run(restart())
        checks.append('real Rust restart cannot revive an old search WebSocket grant')
    finally:server.close()
    result=dict(checks=checks,observed=counts,scope='Real modern relay-cycle socket IO on the authored Lab profile; no original XNet, rendered/played-world, real provider grant or delivery acceptance.')
    if native_probe:result['native']=native(binary,scratch/'native',catalog,Server,native_probe)
    return result


def _await_native_ready(process,timeout=5):
    # The pinned SDK writes this process-start line before main. It is not
    # readiness. Read the pipe directly so selectors and buffered readline
    # cannot disagree about bytes already consumed; retain bytes after READY.
    selector=selectors.DefaultSelector();selector.register(process.stdout,selectors.EVENT_READ)
    deadline=time.monotonic()+timeout;pending=b'';startup=[];received=0
    try:
        while True:
            while b'\n' in pending:
                line,pending=pending.split(b'\n',1);line=line.removesuffix(b'\r')
                if line==b'READY native QoS clients':return startup,pending
                if line==b'Shutdown handler: initialize.' and not startup:
                    startup.append('Shutdown handler: initialize.')
                else:
                    raise RuntimeError('Unexpected native QoS startup control: '+hashlib.sha256(line).hexdigest())
            if received>=4096:raise RuntimeError('Native QoS startup exceeds 4096 bytes')
            remaining=deadline-time.monotonic()
            if remaining<=0 or not selector.select(remaining):raise RuntimeError('Native QoS probe did not publish READY')
            block=os.read(process.stdout.fileno(),4096-received)
            if not block:raise RuntimeError('Native QoS probe exited before READY')
            received+=len(block);pending+=block
    finally:selector.close()


def native(binary,scratch,catalog,Server,probe):
    """Use the existing native OnlineSearchQos clients for both real sockets."""
    scratch.mkdir();state=scratch/'state';server=Server(binary,state,scratch,catalog);process=None
    try:
        entries=json.loads(catalog.read_text())['playlists'];lab=next(e for e in entries if e['id']=='lab-test')
        maps=sorted({g['map'] for e in entries for g in e['games']})
        tokens=['h2s_'+c*43 for c in 'JK']
        db=sqlite3.connect(state/'registry.sqlite3')
        with db:db.executemany('INSERT INTO steam_login_sessions VALUES(?,?,?,?)',[
            (hashlib.sha256(t.encode()).hexdigest(),'7656119796028794'+str(i),'Steam Fixture',int(time.time())+3600) for i,t in enumerate(tokens,5)])
        db.close()
        def expect(method,path,status,**kwargs):
            code,value=server.request(method,path,**kwargs)
            assert code==status,(path,code,{'error':value.get('error'),'fields':sorted(value)})
            return value
        grants=[expect('POST','/v1/matchmaking/session',201,token=t,body=dict(screen_name='Native QoS '+str(i),maps=maps,compatibility='halo2-native-queue-v16',language=0))['token'] for i,t in enumerate(tokens)]
        env=dict(os.environ,H2_TEST_QOS_URL=f'http://{server.host}:{server.port}',H2_TEST_QOS_TOKEN_A=grants[0],H2_TEST_QOS_TOKEN_B=grants[1])
        process=subprocess.Popen([str(probe)],env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        startup,pending=_await_native_ready(process)
        for grant in grants:expect('POST','/v1/matchmaking/ticket',201,token=grant,body=dict(playlist_id=lab['id'],revision=lab['revision'],request_id=str(uuid.uuid4())))
        deadline=time.monotonic()+20;match=None
        while time.monotonic()<deadline:
            for grant in grants:
                row=expect('GET','/v1/matchmaking/ticket?progress=1&start=1',200,token=grant)
                if row['match'] is not None:match=row;break
            if match:break
            time.sleep(.05)
        assert match is not None and len(match['match']['players'])==2,'Native QoS socket clients did not produce the real Lab proposal'
        process.send_signal(signal.SIGINT)
        output=(pending+process.communicate(timeout=5)[0]).decode(errors='replace')
        assert process.returncode==0 and 'PASS native QoS client shutdown' in output
        return dict(passed=True,proposal_players=2,startup=startup,scope='Actual native OnlineSearchQos clients against Rust WebSockets and queue decisions; fixture provider grants only; no room/gameplay acceptance.')
    finally:
        if process and process.poll() is None:
            process.terminate()
            try:process.wait(timeout=5)
            except subprocess.TimeoutExpired:process.kill();process.wait()
        if process and process.stdout:process.stdout.close()
        server.close()
